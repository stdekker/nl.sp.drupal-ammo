# Ammo

Drupal 7 module for submitting and handling amendments (*wijzigingsvoorstellen*)
and motions (*moties*) for SP meetings (*bijeenkomsten*), such as a congress.
Afdelingen submit proposals, other afdelingen co-submit them, the secretariat
assesses them, and afdelingen can request a vote.

How a proposal moves from Concept to Beoordeeld, and what is possible in each
status, is described in [STATUSSES.md](STATUSSES.md).

## Functions

For afdelingen (permission `add amendments and motions`):

- **Submit** amendments and motions, or save them as a concept first
  (`/ammo/amendment`, `/ammo/motion`).
- **Overviews** of all proposals of a meeting, per document and chapter
  (`/ammo/amendments`, `/ammo/motions`), and a page per proposal
  (`/ammo/amendment/view/ID`, `/ammo/motion/view/ID`).
- **Mijn afdeling** (`/ammo/afdeling`): what the afdeling submitted and what
  it co-submitted, with a download of those proposals as one Word file: the
  amendments per document, then the motions when the meeting has them.
- **Change** a submitted proposal, **co-submit** (*mede indienen*) another
  afdeling's proposal, **withdraw**, and **request a vote** (*stemming
  aanvragen*).
- **Mails** on submitting, changing, assessing, withdrawing, co-submitting,
  vote requests and deleting.

For viewers (permission `view submitted amendments and motions`): the
overviews and proposal pages.

For admins (permissions `administer amendments and motions` and `administer
ammo`), under `/admin/ammo`:

- **Bijeenkomsten**: meetings with their periods for submitting and changing,
  the *Uiterste intrek/ondersteuningsdatum*, and switches to hide the meeting,
  the statuses or the advice.
- **Documenten**: the documents to amend, with their locations (*locaties*)
  such as Hoofdstuk, Artikel and Lid that number the amendments (`3.12`,
  `B.4`).
- **Status/advies** per proposal, plus managing owners (*indieners*), vote
  requests and deleting.
- **Status wijzigen** (`/admin/ammo/status`): amendments and motions per
  document and chapter, with their location, submitter, the start of the
  text, status, advice and the same actions as on the proposal itself; tick
  proposals to give them a new status at once.
- **Print** amendments (by location or number) or motions (by afdeling or
  number) to a Word file.
- **Stemmingaanvraag instellingen**: the statuses on which a vote can be
  requested.
- **E-mailinstellingen** (`/admin/ammo/mail-settings`, separate permission
  `administer ammo mail settings`): turn each mail on or off per step,
  separately for the submitters and for the meeting's address. A main switch
  stops all mail and keeps those choices.
- **Testgegevens**: generate and remove test proposals for a meeting.

## Requirements

- Drupal 7 with PHP 7.4.
- The modules `entity`, `date`, `date_popup`, `number` and `shurly`.
- The custom module `procoid`, which links Drupal users to Procurios contacts
  and their afdelingen.

## Installation

1. Place the module in `sites/default/modules/custom/nl.sp.drupal-ammo` and
   enable it.
2. Run `drush updatedb`.
3. Grant the permissions above at `/admin/people/permissions`.

## Deployment

1. Back up the database.
2. Run `drush updatedb` and `drush cc all`.

## Tests

The scripts in `tests/` run against the database inside a transaction and
roll back their fixtures. Run them all from the DDEV project root:

```bash
for t in public_html/sites/default/modules/custom/nl.sp.drupal-ammo/tests/*.test.php; do ddev drush scr "${t#public_html/}" || break; done
```
