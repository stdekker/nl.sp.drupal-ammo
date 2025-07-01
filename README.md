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
Validates amendment values against document constraints.

### `ammo_get_document_constraints_display($document_id)`  
Returns formatted constraints string for display.

## CSS Classes

- `.document-constraints`: Styling for constraint display boxes

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