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
    *   Export meeting data, including accepted amendments and motions, to XML.
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