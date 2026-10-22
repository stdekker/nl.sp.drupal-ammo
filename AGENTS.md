# AGENTS.md — Ammo (nl.sp.drupal-ammo)

Drupal 7 module for submitting and managing amendments (*wijzigingsvoorstellen*)
and motions (*moties*) for SP meetings (*bijeenkomsten*, e.g. a congress).
Machine name `ammo`. See `README.md` for the basic functions and
`STATUSSES.md` for the flow of a proposal through its states.

## Environment

- Drupal 7, PHP 7.4, MariaDB 10.6, run through DDEV. The DDEV project
  (`ammotest`) lives four levels up; the docroot is `public_html`.
- This module is its own git repository (`stdekker/nl.sp.drupal-ammo`). The
  directories above it are not under version control, so run git commands from
  the module directory.
- Run commands through DDEV from the project root, never with a host PHP:
  `ddev drush ...`, `ddev exec php -l ...`.
- Dependencies: `entity`, `date`, `date_popup`, `number`, `shurly`, and the
  custom `procoid` module, which links Drupal users to Procurios contacts and
  their branches (*afdelingen*).

## Language

- Everything users see is Dutch. Most UI strings are hard-coded Dutch, often
  without `t()`. Follow the file you are editing.
- Code identifiers and comments are English. Domain words stay Dutch where the
  code already uses them: `afdeling`, `ammo_get_afdelingen()`,
  `ammo_afdelingen_access()`.

## Layout

| File | Role |
| --- | --- |
| `ammo.module` | Hooks: menu, permissions, access callbacks, `hook_mail`, `hook_theme`, `template_preprocess_*`, `hook_entity_info`, intro and admin pages. |
| `ammo.inc` | Shared helpers: states, advice, afdelingen/contacts, relations, saving (`ammo_save_entity`, `ammo_store`), access (`ammo_edit_access`, `ammo_access`, …), diff and mail helpers, document range validation. Loaded on every request by `ammo_init()`. |
| `ammo.install` | `ammo_schema()` plus `ammo_update_7001`–`7020`. |
| `*.form.inc` | Form builders, validators and submit handlers, one per page (`amendment`, `motion`, `meeting`, `document`, `support`, `ownership`, `vote_request*`, `mail_settings`, `print`, `export`, `branch-overview`, …). |
| `locators.inc` | Location indicators (locators) of documents and amendments: definitions with the legacy fallback, validation, storage fields, grouping, numbering, location text and sorting. Required by `ammo.inc`. |
| `submitted-edit.inc`, `amendments-pager.inc`, `branch-overview.inc`, `status-overview.inc`, `item-view.inc`, `test-data.inc` | Logic for the newer features, kept apart from their form or page files. |
| `*.tpl.php` | Templates registered in `ammo_theme()`; their variables come from the matching `template_preprocess_*` in `ammo.module`. |
| `docx.inc` | A minimal Word (.docx) writer on `ZipArchive`, used by the Mijn afdeling download. The admin print still sends HTML with a .docx name. |
| `ammo.css`, `ammo.js` | Front-end assets. `ammo.css` is added through `ammo.info`. |
| `tests/*.test.php` | Integration checks run with `drush scr`. |

## Data model

Five Entity API entities with no fields (`fieldable => FALSE`), each backed
by its own table:

- `meeting` → `ammo_meetings`. Holds ten date ranges, each a `_start` and
  `_end` column: `submission`, `change`, `withdraw`, `support_withdraw` and
  `vote_request`, each for `amendment_` and `motion_` (see
  `ammo_meeting_ranges()`). It also holds the meeting `date`, and switches
  such as `hide_meeting`, `hide_state` and `hide_advice`. An empty motion
  submission range means the meeting has no motions
  (`ammo_meeting_has_motions()`).
- `document` → `ammo_documents`, which belongs to a meeting. `locators` holds
  a JSON list of location indicators (key, label, type `numeric|letter|text`,
  min, max, grouping, required). When it is NULL the document is legacy and
  uses the ranges `chapter_min/max`, `page_min/max` and `line_max`.
- `amendment` → `ammo_amendments`, which belongs to a document. Legacy
  documents store `chapter`, `page` and `line`; other documents store
  `locator_values` (JSON by locator key) and `group_value`. Its number is
  `group.chapterized_id`, where the group is `COALESCE(group_value, chapter)`.
- `motion` → `ammo_motions`, which belongs to a meeting directly. Its number is
  `motion_id`.
- `contact_relation` → `ammo_contact_relations`. It links a contact to an
  amendment or motion:
  - `target_type` and `target_id` name the amendment or motion.
  - `relation_type` is `owner`, `backer` or `vote_request`.
  - `contact_type` is `member` or `branch`.
- `ammo_test_entries` tracks generated test data. It is a plain table, not an
  entity.

The rules the code depends on:

- **States** come from `ammo_states()`: `concept`, `submitted`, `changed`, `assessed`,
  `withdrawn`, `invalid`, `blocked`, `spelling_grammar`.
  - `assessed` ("Beoordeeld") replaced `accepted` and `rejected` in update
    7015. The outcome is the advice (`accept` or `reject`); test it with
    `ammo_assessed_as($item, 'reject')`, never with the state alone.
  - `blocked` (shown as "Verwijderd") is visible to admins only. Admins set it
    through the delete confirmation page (`ammo_delete_entity_form()`), not the
    status dropdown; use `ammo_editable_states()` for state selects.
  - A `concept` is visible to admins and to its owners.
- **First owner**: the owner relation with the lowest id is the submitter, and
  member owners take precedence over branch owners (`ammo_get_first_owner()`).
  Every other branch owner is a co-submitter (*mede-indiener*).
  `branch-overview.inc` repeats this rule in SQL, so keep the two in sync.
- **Hidden meetings** (`hide_meeting`) show nothing to anyone.
- **Dates**: amendments and motions store the time of their first save in
  `created` and of their last save in `changed`, both set by
  `ammo_entity_presave()`. They are empty for items created before update 7019
  or not saved since update 7018; show them with `ammo_item_date()`.
- **Locators**: read them through `ammo_document_locators()` and
  `ammo_amendment_locator_values()`, which build chapter, page and line for
  legacy documents. Use `ammo_amendment_number()`, `ammo_amendment_group()`
  and `ammo_next_chapterized_id()` instead of `chapter` directly. A document
  with amendments may not change its locator structure.

## Access

The module defines five permissions: `administer ammo` (superadmin),
`administer amendments and motions` (admin), `add amendments and motions`,
`view submitted amendments and motions`, and `administer ammo mail settings`.
The last one stands apart: the admin permissions do not include it.

- Use the existing helpers instead of calling `user_access()` directly:
  - `ammo_superadmin_access()` and `ammo_admin_access()` for admin rights.
  - `ammo_mail_settings_access()` for the mail settings page.
  - `ammo_add_access()` and `ammo_view_overview_access()` for submitting and
    viewing.
  - `ammo_*_motions_access()` for motion pages.
  - `ammo_edit_access()`, `ammo_co_submit_access()`,
    `ammo_withdraw_access()`, `ammo_support_withdraw_access()`,
    `ammo_support_access()`, `ammo_vote_request_access()` and
    `ammo_advice_access()` for the actions on one entry. Each takes the type
    and follows its range: co-submitting the submission range, withdrawing
    `_withdraw`, leaving as co-submitter, removing member submitters and
    member support `_support_withdraw`, and vote requests `_vote_request`.
    `ammo_withdrawable_branches()` sorts a user's owner branches into first
    owner (withdraw) and co-submitters (leave). The status/advies form closes
    at the meeting `date`. Vote requests are further limited to the statuses
    in `ammo_vote_request_allowed_statuses()`; admins bypass both limits.
- Submitting and changing follow the meeting ranges through
  `ammo_meeting_range_open()`; `ammo_edit_access()` applies them. A concept
  follows the submission range, a submitted item the change range, and only
  the first owner's branch may change it. Admins bypass the ranges.
  - The owner's change to a submitted item goes through
    `ammo_save_submitted_edit()` in `submitted-edit.inc`. It is saved in place
    with the state `changed` ("Gewijzigd"), the advice is kept, and every owner
    branch is mailed.
- The meeting date (`event`) is enforced through `ammo_passed_date()`.
  Admins can bypass some deadlines, and the existing checks show which ones.
- A user's branches come from `ammo_get_afdelingen($contact_id)`.
  - The result is cached in `drupal_static` and in the `ammo_data` cache entry
    for 24 hours.
  - Pass the special key `'all'` to get every branch.
  - The contact id is read from the user's `field_contact_id`.

## Conventions

- Match the surrounding Drupal 7 style:
  - Two-space indentation.
  - `array()` in the older code. Some newer code uses `[]`.
  - Function names prefixed with `ammo_`.
  - Docblocks of a single sentence on new functions.
- To add a page:
  1. Add a `hook_menu` entry in `ammo_menu()` whose `'file'` key points to the
     include that holds the callback.
  2. Use one of the access callbacks above.
  3. If the page is a form, use `drupal_get_form` as the page callback.
- To add a theme template:
  1. Register it in `ammo_theme()`.
  2. Put its data preparation in a `template_preprocess_<hook>()` in
     `ammo.module` or in the feature's `.inc` file.
  3. Keep the logic out of the `.tpl.php`.
- Larger features follow this split: `feature.inc` holds the logic and the
  queries, `feature.form.inc` the form or page, `feature.tpl.php` the markup.
  Use `module_load_include('inc', 'ammo', 'feature')` where it is needed.
- Escape output:
  - Pass user-entered body text through `ammo_sanitize_text()`.
  - Use `check_plain()` for plain values.
  - Use `l()` or `url()` for links.
  - Use `ammo_item_view_path($type, $id)` to link to the page of one item.
- Mail goes through `hook_mail` (`ammo_mail()`) and the `ammo_*_mail()`
  helpers in `ammo.inc`. Do not send mail from test data or from test scripts.
  - Each helper belongs to a step from `ammo_mail_steps()` and sends through
    `ammo_send_mail($step, $recipient, …)`. The recipient is `user` (the
    contacts of the proposal) or `admin` (the meeting's address).
  - Users with `administer ammo mail settings` switch steps per recipient at
    `admin/ammo/mail-settings`. The settings are stored in `ammo_mail_steps`,
    and a step that was never saved follows `ammo_mail_step_defaults()`. The main switch `ammo_mail_enabled`
    stops all mail but keeps the steps.
  - A new mail needs a new step in `ammo_mail_steps()`. `ammo_store()` picks
    its step itself (`ammo_store_mail_step()`); the other callers pass theirs.
  - A switched-off mail counts as sent, so callers don't warn about it.
- To change the schema:
  1. Change `ammo_schema()`.
  2. Add the next `ammo_update_70NN()` (currently 7021). Guard it with
     `db_field_exists()` or `db_table_exists()`, and reuse the definition from
     `ammo_schema()`.
- To change data access, prefer `db_select` and `EntityFieldQuery`. Load and
  save entities with `entity_load_single()` and `ammo_save_entity()`.

## Testing

There is no PHPUnit or SimpleTest suite. Each file in `tests/` is a script
that runs against the DDEV database inside a transaction and rolls back all of
its fixtures. Run one from the project root:

```bash
ddev drush scr sites/default/modules/custom/nl.sp.drupal-ammo/tests/item-view.test.php
```

Run all of them:

```bash
for t in public_html/sites/default/modules/custom/nl.sp.drupal-ammo/tests/*.test.php; do ddev drush scr "${t#public_html/}" || break; done
```

A passing script prints `PASS: …`. A failing one throws an exception with the
message of the assertion that failed.

New tests follow the same pattern:

1. Save `$user` and `$_GET`, then open `db_transaction()`.
2. Inside a `try` block, create fixtures with `ammo_save_entity()`, assert
   with a small local `*_assert()` helper that throws, then call
   `$transaction->rollback()` and print a `PASS:` line.
3. In the `catch (Throwable $e)` block, roll back and rethrow.
4. In the `finally` block, restore `$user` and `$_GET`.

- Fake permissions and branches by writing into `drupal_static('user_access')`
  and `drupal_static('ammo_get_afdelingen')`.
- Use contact and branch ids in the `999999xx` range.
- Before finishing, lint the PHP files you changed with
  `ddev exec php -l /var/www/html/public_html/sites/default/modules/custom/nl.sp.drupal-ammo/<file>`
  and run the tests that touch the area you changed. `ddev exec` starts in the
  docroot, so a path relative to the project root does not work.

## Deployment notes

- After a change to menu items or theme hooks, run `ddev drush cc all`.
- After a schema change, run `ddev drush updatedb`, and take a database backup
  first.
- When a feature changes user-visible behaviour, keep `README.md` (a short
  list of functions) and `STATUSSES.md` (what is possible in each state) up to
  date. Put the drush steps of a change in its commit message, not in the
  README.

## Known quirks

- The export page (`admin/ammo/export`) is disabled in the admin overview
  ("TEMPORARILY DISABLED").
- In `ammo_show_motions()`, `isset($result)` checks the wrong variable, so the
  static cache never takes effect.
- `devel/ammo/afdelingen` is a debug page that shows the branches of the
  current user.
