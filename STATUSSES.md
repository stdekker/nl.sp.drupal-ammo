# Statuses of a proposal

Every amendment (*wijzigingsvoorstel*) and motion (*motie*) has one status.
This page describes what each status means, who moves a proposal from one
status to the next, and what is still possible in each status.

*Admins* are users with the permission to manage all amendments and motions
(the secretariat). *Submitter* (*hoofdindiener*) is the afdeling that submitted
the proposal; *co-submitters* (*mede-indieners*) are afdelingen that joined it
later.

The mails described below can be switched on and off per recipient by users
with the permission `administer ammo mail settings` at
`/admin/ammo/mail-settings` (*E-mailinstellingen*), or all at once. Until those
settings are saved, only the owner afdelingen are mailed, and only when the
submitter changes an assessed proposal, on co-submitting and leaving, on
withdrawing, on vote requests and their withdrawal, and on deleting. The
meeting's address gets no mail by default.

## The flow

```mermaid
stateDiagram-v2
  state "Tekstuele wijziging" as TekstueleWijziging
  [*] --> Concept: save as concept
  [*] --> Ingediend: submit
  Concept --> Ingediend: submit
  Ingediend --> Beoordeeld: admin gives an advice
  Gewijzigd --> Beoordeeld: admin assesses again
  Beoordeeld --> Gewijzigd: submitter changes the text (only after Afwijzen)
  Ingediend --> Ingetrokken: submitter withdraws
  Gewijzigd --> Ingetrokken: submitter withdraws
  Beoordeeld --> Ingetrokken: submitter withdraws (amendments, only after Afwijzen)
  Ingediend --> Ongeldig: admin
  Ingediend --> TekstueleWijziging: admin
  Ongeldig --> Ingetrokken: submitter withdraws
  Ingediend --> Verwijderd: admin deletes
  Verwijderd --> Ingediend: admin restores
```

Admins can set any status (except Verwijderd) on the **status/advies** form at
any time until the meeting starts, so the diagram shows the usual route, not
every possible one. On **Status wijzigen** (`/admin/ammo/status`) they can give
several proposals the same status at once; Beoordeeld still needs an advice,
and each change is mailed as on the status/advies form.

## The statuses

| Status | Meaning | Who sees it |
| --- | --- | --- |
| **Concept** | Saved but not yet submitted. | Admins and the owners of the proposal. |
| **Ingediend** | Submitted, waiting for an assessment. | Everyone who can view proposals. |
| **Gewijzigd** | The submitter changed the text after submitting; waiting for a new assessment. | Everyone who can view proposals. |
| **Beoordeeld** | Assessed. The advice gives the outcome: *Overnemen* or *Afwijzen*. | Everyone who can view proposals. |
| **Ingetrokken** | Withdrawn. Shown folded in. | Everyone who can view proposals. |
| **Ongeldig** | Not valid. Amendments are shown folded in. | Everyone who can view proposals. |
| **Tekstuele wijziging** | A change in spelling or grammar only. | Everyone who can view proposals. |
| **Verwijderd** | Deleted by an admin. | Admins only. |

A meeting can hide statuses and advice from regular users. Concept,
Ingetrokken and (for amendments) Ongeldig are always shown.

The advice is never shown while a proposal is Concept or Ingediend, to admins
neither: not on the proposal, in its mails or on Mijn afdeling. The download on
Mijn afdeling follows the same rules as that page, hidden statuses and advice
included. The admin print and export still include the status and advice.

## Step by step

### 1. Concept

An afdeling starts a proposal and can save it as a concept. It can keep
changing the concept while the submission period (*Indienen*) of the meeting is
open. Only its owners and admins see it.

### 2. Ingediend

When the afdeling submits the proposal it gets the status **Ingediend**, and
the afdeling gets a confirmation mail. While a proposal is Ingediend:

- other afdelingen can **co-submit** it (*mede indienen*) while the *Indienen*
  period is open; this is only possible in this status;
- members can support it while the *Steun intrekken* period is open;
- the submitter cannot change it yet: that is only possible once it is
  Beoordeeld (see Gewijzigd);
- the submitter can **withdraw** it while the *Voorstel intrekken* period is
  open, and co-submitters can leave it while the *Steun intrekken* period is
  open.

### 3. Gewijzigd

When the submitter changes an assessed proposal, only the text and the
explanation (*toelichting*) can change; the number and location stay the same.
The proposal gets the status **Gewijzigd** and keeps its previous advice. The
secretariat and all owner afdelingen, co-submitters included, get a mail with
the differences.

A Gewijzigd proposal cannot be changed again until an admin has assessed it
again. Other afdelingen cannot co-submit it.

### 4. Beoordeeld

An admin assesses the proposal on the **status/advies** form. The status
Beoordeeld always needs an advice:

- **Overnemen** (accept): the proposal is final. Its afdelingen can no longer
  change or withdraw it. Only afdelingen that did not (co-)submit it can request
  a vote.
- **Afwijzen** (reject): the submitter can still change it (it becomes
  Gewijzigd) while the change period is open. Any afdeling can request a vote.
  A rejected amendment can still be withdrawn but gets no new member support. A
  rejected motion is shown folded in and can no longer be withdrawn by its
  afdelingen.

The secretariat and the owner afdelingen get a mail when the status or advice
changes, unless the meeting hides it. A vote can be requested (*stemming aanvragen*) on the statuses chosen under
**Stemmingaanvraag instellingen**; by default that is only Beoordeeld.

### 5. Other statuses

- **Ingetrokken**: the submitter withdrew the proposal (also when it has
  co-submitters), or the last owner left it. It is shown folded in, and
  afdelingen can do nothing more with it.
- **Ongeldig**: set by an admin. It can no longer be co-submitted or changed,
  and an amendment gets no new member support. The submitter can still
  withdraw it.
- **Tekstuele wijziging**: set by an admin for a change in spelling or grammar
  only. Afdelingen can no longer withdraw, change or co-submit it.
- **Verwijderd**: an admin deletes the proposal with the **Verwijderen** button
  and an explanation, which is mailed to the owners. Only admins still see it.
  Choosing another status on status/advies restores it.

## Deadlines

Each meeting has these periods (*termijnen*) separately for amendments and for
motions. A period runs from the start of its Van day to the end of its Tot day;
without Van it is open right away, and an empty period means the action is not
possible.

| Deadline | What it opens |
| --- | --- |
| *Indienen* period | Submitting new proposals, changing concepts and co-submitting (*mede indienen*). |
| *Wijzigen na indienen* period | Changing proposals assessed as Afwijzen by the submitter. |
| *Voorstel intrekken* period | Withdrawing the proposal by the submitting afdeling. |
| *Steun intrekken* period | Leaving as co-submitter, removing individual member submitters, and member support. |
| *Stemming aanvragen* period | Requesting a vote and withdrawing that request. |
| Meeting date | Closes the status/advies form. |

Admins are not bound by the *Indienen* and *Wijzigen* periods, and can still
handle vote requests outside the *Stemming aanvragen* period.
