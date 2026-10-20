# AMMO Module - Amendment and Motion Management

This Drupal module manages amendments and motions for meetings.

## Features

- Meeting management
- Document management with validation ranges
- Amendment submission and management  
- Motion submission and management
- Contact and branch relationship management
- Email notifications
- Print functionality
- Export functionality

## Document Validation Ranges

As of version 7011, documents now support validation ranges for amendments:

### Document Settings
When creating or editing a document, you can set:
- **Chapter range**: Minimum and maximum chapter numbers (e.g., 1-7)
- **Page range**: Minimum and maximum page numbers (e.g., 1-123)  
- **Line maximum**: Maximum line number allowed

### Amendment Validation
When creating or editing amendments, the system validates:
- Chapter number must be within the document's chapter range
- Page number must be within the document's page range
- Line number must not exceed the document's maximum line number

The amendment form displays the allowed ranges for user guidance.

### Database Changes
New fields added to `ammo_documents` table:
- `chapter_min`: Minimum chapter number (default: 1)
- `chapter_max`: Maximum chapter number
- `page_min`: Minimum page number (default: 1)  
- `page_max`: Maximum page number
- `line_max`: Maximum line number

## Installation

1. Place module in `sites/default/modules/custom/nl.sp.drupal-ammo`
2. Enable the module
3. Run database updates: `drush updatedb`

## Database Updates

Run the following to apply validation range updates:
```bash
drush updatedb
```

This will execute update 7011 to add the new validation fields.

## Usage

The amendment overview loads one document/chapter at a time. Chapter links
above and below the entries use `?document=ID&chapter=N` and work without
JavaScript. Chapter 0 groups entries without a chapter; invalid selections
fall back to the first visible chapter. Admin totals still cover the document.
Run the rollback-only check with
`ddev drush scr sites/default/modules/custom/nl.sp.drupal-ammo/tests/amendments-pager.test.php`.

### Afdelingsoverzicht

`/ammo/afdeling` (tab **Mijn afdeling**, requires `add amendments and motions`)
lists per afdeling of the user what it submitted and, in a second part, what it
co-submitted. The first owner of an entry is the submitter; every other branch
owner is a mede-indiener. Own concepts are listed with the submitted entries.
Each number links to the page of the entry. Motions are listed when the
meeting has them.
Admins can pick any afdeling (`?afdeling=ID`). After deployment rebuild the
menu and theme registry with `drush cc all`; there is no database update.
Run the rollback-only check with
`ddev drush scr sites/default/modules/custom/nl.sp.drupal-ammo/tests/branch-overview.test.php`.

### Itempagina

Every amendment and motion has its own page: `/ammo/amendment/view/ID` and
`/ammo/motion/view/ID`. It shows the entry with the same action links as the
overviews; actions return to the page. Access follows the overviews: the view
or add permission is required, blocked entries are for admins only, concepts
for admins and their owners (member or afdeling), and nothing is shown of a
hidden meeting. Notification mails and the afdelingsoverzicht link to this
page. After deployment rebuild the menu with `drush cc all`.
Run the rollback-only check with
`ddev drush scr sites/default/modules/custom/nl.sp.drupal-ammo/tests/item-view.test.php`.

### Beoordeeld

The statuses *Overgenomen* and *Afgewezen* are replaced by *Beoordeeld*. The
outcome of the assessment is the advice (*Overnemen* or *Afwijzen*), so the
status/advies form refuses *Beoordeeld* without an advice. Where the
module treated the old statuses differently, it now follows the advice:

- amendments assessed with *Afwijzen* get no new support from regular users;
- motions assessed with *Afwijzen* are folded in and lose their withdraw link,
  and stay visible in Mijn afdeling when states are hidden;
- the XML export contains items assessed with *Overnemen*;
- vote requests default to *Beoordeeld* (see the vote request settings).
- on items assessed with *Overnemen* only branches that are not the indiener
  or a mede-indiener (also not the branch of a member indiener) can request a
  vote; on *Afwijzen* any branch can. Admins are not bound by this. Run
  `ddev drush scr sites/default/modules/custom/nl.sp.drupal-ammo/tests/vote-request.test.php`.

After deployment back up the database and run `drush updatedb`. Ammo update
7015 sets existing Overgenomen and Afgewezen items to Beoordeeld, fills an
empty advice from the old status, maps the vote request setting, and reports
items whose advice contradicts their old status (also in the watchdog log).

### Termijnen

Each meeting has four date ranges (*termijnen*) on its admin form, each with a
**Van** and a **Tot** date:

- **Indienen wijzigingsvoorstellen**: new amendments can be submitted. Required.
- **Indienen moties**: new motions can be submitted. Empty means the meeting
  has no motions; this replaces the old *Geen moties* checkbox.
- **Wijzigen ingediende wijzigingsvoorstellen**: the submitter can change a
  submitted amendment. Empty means it cannot be changed.
- **Wijzigen ingediende moties**: the same for motions.

A range without Van is open right away; Van without Tot is refused. A concept
follows the submission range of its type. Only the submitting branch (the
first owner) can change a proposal; co-submitting branches cannot. Admins are
not bound by the ranges. The *Uiterste intrek/ondersteuningsdatum* is a date
too and runs to the end of its day. Withdrawing, supporting and vote requests
close then; supporting used to stay open until the meeting started. The
status/advies form stays open until the meeting starts.

An item with the status Beoordeeld and the advice Overnemen can no longer be
changed or withdrawn by its branches; admins still can. A change by the submitter is saved
on the proposal itself. Through **voorstel
wijzigen** the submitter changes only the text and the toelichting; the
location and number stay as submitted (admins can still change the
location). A change without differences is refused, and a change of less
than 5% of the text shows a warning before it is confirmed (for motions the
warning asks to submit again). It gets the status **Gewijzigd** and keeps its
advice until an admin assesses it again; until then the submitter cannot
change it again. The secretariat and all owner branches, co-submitters
included, get a mail with the differences. Items with the status Gewijzigd are
left out of the XML export and vote requests until they are assessed again.

After deployment back up the database, run `drush updatedb` and `drush cc
all`. Ammo update 7016 adds the ranges and fills their Tot dates from the old
settings: both submission ranges from the old submission deadline (no motion
range for meetings with *Geen moties*), and the amendment change range up to
the meeting date where *Ingediende wijzigingsvoorstellen wijzigen toestaan* was
on. Every end, the withdrawal date included, is moved to the end of its day
(23:59:59). It then drops `submission_closing_date`, `no_motions` and
`allow_submitted_amendment_edit`. Run the rollback-only checks with
`ddev drush scr sites/default/modules/custom/nl.sp.drupal-ammo/tests/meeting-ranges.test.php`
and `.../tests/submitted-edit.test.php`.

### Toelichting

The status/advies form has one **Toelichting** field instead of *Toelichting
statuswijziging* and *Toelichting advies*. Item pages and overviews show it
under its own heading right after the advice, with the same visibility as the
advice (hidden when the meeting hides advice, except for admins). The
submitter's own explanation is now headed *Toelichting voorstel* on motions
too. When the submitter changes a submitted item, the Toelichting stays, like
the advice.

After deployment back up the database, run `drush updatedb` and `drush cc
all`. Ammo update 7017 moves the old status explanation into the Toelichting
(after the advice explanation, separated by a blank line, unless the two are
the same) and drops `state_supplement` from `ammo_amendments` and
`ammo_motions`. Run the rollback-only check with
`ddev drush scr sites/default/modules/custom/nl.sp.drupal-ammo/tests/submitted-edit.test.php`.

### Verwijderen

The status *Verwijderd* is no longer in the status dropdown of **status/advies**.
Users who can manage all amendments and motions (`administer amendments and
motions`) get a **Verwijderen** button there instead. It opens a confirmation
page (`/ammo/amendment/delete/ID`, `/ammo/motion/delete/ID`) that asks for an
explanation, which is mailed to the submitters. The item gets the status
Verwijderd and is then only visible to admins. Choosing another status on
status/advies restores it; for a deleted item *Verwijderd* stays in the
dropdown. There is no database update; run `drush cc all` after deployment.
Run the rollback-only check with
`ddev drush scr sites/default/modules/custom/nl.sp.drupal-ammo/tests/delete.test.php`.

### Locaties (locators)

New documents describe where an amendment applies with a configurable list of
locations (*locaties*). Under **Locaties** on the document form each row has a
name, a type (*Getal* with an optional minimum and maximum, *Letter (A–Z)* with
an optional letter range, or *Vrije tekst*), a *Verplicht* checkbox and an
order. A new document starts with Hoofdstuk, Pagina and Regelnummer. Exactly
one location is *Groeperend*: it must be a number or a letter, is always
required, and drives the overview tabs and the numbering (`3.12`, `B.4`).
Clear a name to remove a location.

Once a document has amendments only names, ranges, *Verplicht* and the order
can change; adding, removing, changing a type or the grouping location is
refused.

Documents created before this version keep the chapter, page and line columns
and their "Validatie instellingen"; their amendments, numbers and output are
unchanged. No data is migrated.

After deployment back up the database, run `drush updatedb` (Ammo update 7014
adds `ammo_documents.locators`, `ammo_amendments.locator_values` and
`ammo_amendments.group_value`) and `drush cc all` for the new theme hook.
Run the rollback-only check with
`ddev drush scr sites/default/modules/custom/nl.sp.drupal-ammo/tests/locators.test.php`.

### Setting Document Validation Ranges

1. Go to document management
2. Create or edit a document  
3. Configure validation ranges in the "Validatie instellingen" section
4. Save the document

### Creating Amendments with Validation

1. Create a new amendment
2. Select the document
3. Enter chapter, page, and line numbers
4. The form will show allowed ranges and validate your input
5. Submit the amendment

The system will prevent submission if values are outside the configured ranges.

## API Functions

### `ammo_validate_document_ranges($document_id, $chapter, $page, $line)`
Validates legacy chapter, page and line values against document constraints.

### `ammo_validate_locators($document, $values)`
Validates amendment input against the locations of any document; errors are
keyed by location key. See `locators.inc` for the other location helpers.

### `ammo_get_document_constraints_display($document_id)`  
Returns formatted constraints string for display.

## CSS Classes

- `.document-constraints`: Styling for constraint display boxes

## Meeting test data

After deployment, run database updates (Ammo update 7013) and rebuild the menu
cache. Back up the database before updating.

Under `/admin/ammo/meeting`, choose **Testgegevens** for a meeting. Add 10, 25,
50 or 100 amendments to a selected document, or motions when enabled. Each
series cycles through the existing statuses and uses labelled Dutch sample
text. Existing published, recognized branches with valid branch IDs are assigned
as owners in alphabetical rotation. Generation requires an available branch.
Amendments rotate across chapters 1–10 within the document's configured limits.
No contacts are created and no email is sent. Older test series remain unchanged;
delete and regenerate them to assign branches.

**Alle testgegevens verwijderen** asks for confirmation and removes every
tracked series for that meeting, including contact relations. Real entries
and other meetings remain intact. Edited test entries are still tracked.

Run the rollback-only integration check from the DDEV project:
`ddev drush scr sites/default/modules/custom/nl.sp.drupal-ammo/tests/test-data.test.php`.

# Ammo Module

The Ammo module provides a comprehensive system for managing amendments and motions within a Drupal 7 website, specifically tailored for the needs of the SP (Socialist Party). It allows users to submit, view, and manage amendments and motions related to specific meetings and documents.

## Key Features

*   **Custom Entities:** Defines a robust data model using custom entities:
    *   **Meeting:** Represents an event where motions and amendments are discussed.
    *   **Document:** A document that can be amended.
    *   **Amendment:** A proposed change to a document.
    *   **Motion:** A formal proposal put to a meeting.
    *   **Contact Relation:** Tracks the relationship between contacts (users or branches) and an amendment/motion (e.g., owner, backer).
*   **User-Friendly Interface:**
    *   A dedicated section (`/ammo`) for users to submit and view amendments and motions.
    *   Clear separation between submitting amendments and motions.
    *   Overviews of all submitted proposals.
*   **Administrative Backend:**
    *   An admin section (`/admin/ammo`) for managing meetings and documents.
    *   Tools for printing and exporting data.
*   **Fine-Grained Permissions:**
    *   `administer ammo`: Full control over the module's settings and data.
    *   `administer amendments and motions`: Allows management of submitted proposals.
    *   `add amendments and motions`: Grants users the ability to submit new proposals.
*   **Email Notifications:** Automatically sends email notifications for key events, such as:
    *   Submitting a new proposal.
    *   Modifying an existing proposal.
    *   Withdrawing a proposal.
    *   Supporting a proposal.
*   **Data Export and Printing:**
    *   Export meeting data, including amendments and motions assessed with the advice to accept, to XML.
    *   Generate printable views of amendments and motions, with various sorting options (by location, by number, etc.).
*   **Theming and Customization:** Provides a set of theme templates that can be overridden for custom styling and layout.

## Dependencies

*   Drupal 7
*   [Entity API](https://www.drupal.org/project/entity)
*   [Date](https://www.drupal.org/project/date)
*   [Number](https://www.drupal.org/project/number)
*   [Shurly](https://www.drupal.org/project/shurly)
*   [Swiftmailer](https://www.drupal.org/project/swiftmailer)
*   `procoid` (custom module)

## Installation

1.  Ensure all dependent modules are installed and enabled.
2.  Install the Ammo module as you would any other Drupal module.
3.  Grant the necessary permissions to the appropriate user roles at `/admin/people/permissions`.

## Usage

*   **Users:** Logged-in users with the `add amendments and motions` permission can visit `/ammo` to start submitting proposals.
*   **Administrators:** Users with administrative permissions can manage meetings and documents at `/admin/ammo`.

This module is tightly integrated with the `procoid` module, which handles user authentication and synchronization with the Procurios system. User data from Procurios, such as branch information, is used to determine ownership and support for proposals.
