# AGENTS.md — Ammo (nl.sp.drupal-ammo)

Drupal 7 module for submitting and managing amendments (*wijzigingsvoorstellen*)
and motions (*moties*) for SP meetings (*bijeenkomsten*, e.g. a congress).
Machine name `ammo`. See `README.md` for the feature list and per-feature
deployment notes.

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
| `ammo.install` | `ammo_schema()` plus `ammo_update_7001`–`7014`. |
| `*.form.inc` | Form builders, validators and submit handlers, one per page (`amendment`, `motion`, `meeting`, `document`, `support`, `ownership`, `vote_request*`, `print`, `export`, `branch-overview`, …). |
| `locators.inc` | Location indicators (locators) of documents and amendments: definitions with the legacy fallback, validation, storage fields, grouping, numbering, location text and sorting. Required by `ammo.inc`. |
| `amendment-edit.inc`, `amendments-pager.inc`, `branch-overview.inc`, `item-view.inc`, `test-data.inc` | Logic for the newer features, kept apart from their form or page files. |
| `*.tpl.php` | Templates registered in `ammo_theme()`; their variables come from the matching `template_preprocess_*` in `ammo.module`. |
| `ammo.css`, `ammo.js` | Front-end assets. `ammo.css` is added through `ammo.info`. |
| `tests/*.test.php` | Integration checks run with `drush scr`. |

## Data model

Five Entity API entities with no fields (`fieldable => FALSE`), each backed
by its own table:

- `meeting` → `ammo_meetings`. Holds the deadlines (`submission_closing_date`,
  `withdrawal_closing_date`, `date`) and switches such as `hide_meeting`,
  `no_motions`, `hide_state`, `hide_advice` and
  `allow_submitted_amendment_edit`.
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

- **States** come from `ammo_states()`: `concept`, `submitted`, `accepted`,
  `rejected`, `withdrawn`, `invalid`, `blocked`, `spelling_grammar`.
  - `blocked` (shown as "Verwijderd") is visible to admins only. Admins set it
    through the delete confirmation page (`ammo_delete_entity_form()`), not the
    status dropdown; use `ammo_editable_states()` for state selects.
  - A `concept` is visible to admins and to its owners.
- **First owner**: the owner relation with the lowest id is the submitter, and
  member owners take precedence over branch owners (`ammo_get_first_owner()`).
  Every other branch owner is a co-submitter (*mede-indiener*).
  `branch-overview.inc` repeats this rule in SQL, so keep the two in sync.
- **Hidden meetings** (`hide_meeting`) show nothing to anyone.
- **Locators**: read them through `ammo_document_locators()` and
  `ammo_amendment_locator_values()`, which build chapter, page and line for
  legacy documents. Use `ammo_amendment_number()`, `ammo_amendment_group()`
  and `ammo_next_chapterized_id()` instead of `chapter` directly. A document
  with amendments may not change its locator structure.

## Access

The module defines four permissions: `administer ammo` (superadmin),
`administer amendments and motions` (admin), `add amendments and motions`, and
`view submitted amendments and motions`.

- Use the existing helpers instead of calling `user_access()` directly:
  - `ammo_superadmin_access()` and `ammo_admin_access()` for admin rights.
  - `ammo_add_access()` and `ammo_view_overview_access()` for submitting and
    viewing.
  - `ammo_*_motions_access()` for motion pages.
  - `ammo_edit_access()`, `ammo_support_access()`, `ammo_withdraw_access()`
    and `ammo_vote_request_access()` for the actions on one entry.
- Deadlines are enforced through `ammo_passed_date()`. Admins can bypass some
  of them, and the existing checks show which ones.
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
- To change the schema:
  1. Change `ammo_schema()`.
  2. Add the next `ammo_update_70NN()` (currently 7015). Guard it with
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
- When a feature changes user-visible behaviour, add or update its section in
  `README.md`, with the drush steps and the test command.

## Known quirks

- The export page (`admin/ammo/export`) is disabled in the admin overview
  ("TEMPORARILY DISABLED").
- In `ammo_show_motions()`, `isset($result)` checks the wrong variable, so the
  static cache never takes effect.
- `devel/ammo/afdelingen` is a debug page that shows the branches of the
  current user.
