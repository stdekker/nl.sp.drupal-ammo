<?php

/**
 * Location indicators (locators) that place an amendment in its document.
 *
 * Documents with a NULL `locators` column use the legacy chapter, page and
 * line columns. Their locators are built from those columns, keyed by the
 * column name, so legacy forms, diffs and storage keep working unchanged.
 */

/**
 * The locator types with their Dutch names.
 */
function ammo_locator_types() {
  return array(
    'numeric' => 'Getal',
    'letter' => 'Letter (A–Z)',
    'text' => 'Vrije tekst',
  );
}

/**
 * The locators a new document starts with.
 */
function ammo_default_locators() {
  return array(
    'loc1' => array('key' => 'loc1', 'label' => 'Hoofdstuk', 'type' => 'numeric', 'min' => '1', 'max' => '', 'grouping' => TRUE, 'required' => TRUE),
    'loc2' => array('key' => 'loc2', 'label' => 'Pagina', 'type' => 'numeric', 'min' => '1', 'max' => '', 'grouping' => FALSE, 'required' => TRUE),
    'loc3' => array('key' => 'loc3', 'label' => 'Regelnummer', 'type' => 'numeric', 'min' => '', 'max' => '', 'grouping' => FALSE, 'required' => TRUE),
  );
}

/**
 * Whether a document uses the legacy chapter, page and line columns.
 */
function ammo_document_is_legacy($document) {
  return empty($document->locators);
}

/**
 * The chapter, page and line locators of a legacy document.
 */
function ammo_legacy_locators($document) {
  $value = function ($field) use ($document) {
    return isset($document->$field) && $document->$field !== '' ? (string) $document->$field : '';
  };
  return array(
    'chapter' => array('key' => 'chapter', 'label' => 'Hoofdstuk', 'inline' => 'hoofdstuk', 'title' => 'Hoofdstuk nr.', 'error_label' => 'Hoofdstuk nr.', 'type' => 'numeric', 'min' => $value('chapter_min'), 'max' => $value('chapter_max'), 'grouping' => TRUE, 'required' => TRUE, 'legacy' => TRUE),
    'page' => array('key' => 'page', 'label' => 'Pagina', 'inline' => 'pagina', 'title' => 'Pagina nummer', 'error_label' => 'Pagina nr.', 'type' => 'numeric', 'min' => $value('page_min'), 'max' => $value('page_max'), 'grouping' => FALSE, 'required' => TRUE, 'legacy' => TRUE),
    'line' => array('key' => 'line', 'label' => 'Regel', 'inline' => 'regel', 'title' => 'Regel nummer (begin)', 'error_label' => 'Regelnummer', 'type' => 'numeric', 'min' => '', 'max' => $value('line_max'), 'grouping' => FALSE, 'required' => TRUE, 'legacy' => TRUE),
  );
}

/**
 * The ordered locators of a document (object or id), keyed by locator key.
 */
function ammo_document_locators($document) {
  if (is_scalar($document) && !empty($document)) {
    $document = entity_load_single('document', $document);
  }
  if (!is_object($document)) {
    return ammo_legacy_locators(NULL);
  }
  if (!ammo_document_is_legacy($document)) {
    $decoded = json_decode($document->locators, TRUE);
    if (is_array($decoded) && $decoded) {
      $locators = array();
      foreach ($decoded as $locator) {
        $locator += array('label' => '', 'type' => 'text', 'min' => '', 'max' => '', 'grouping' => FALSE, 'required' => FALSE);
        $locator['min'] = (string) $locator['min'];
        $locator['max'] = (string) $locator['max'];
        $locator['grouping'] = !empty($locator['grouping']);
        $locator['required'] = !empty($locator['required']) || $locator['grouping'];
        $locators[$locator['key']] = $locator;
      }
      return $locators;
    }
  }
  return ammo_legacy_locators($document);
}

/**
 * The grouping locator of a set of locators, or NULL.
 */
function ammo_grouping_locator(array $locators) {
  foreach ($locators as $locator) {
    if (!empty($locator['grouping'])) {
      return $locator;
    }
  }
  return NULL;
}

/**
 * Whether the locator structure of a document can no longer change.
 */
function ammo_document_locators_locked($document) {
  if (empty($document->id)) {
    return FALSE;
  }
  return (bool) db_query_range('SELECT 1 FROM {ammo_amendments} WHERE document_id = :id', 0, 1, array(':id' => $document->id))->fetchField();
}

/**
 * The name of a locator inside a sentence.
 */
function ammo_locator_inline_label($locator) {
  return isset($locator['inline']) ? $locator['inline'] : $locator['label'];
}

/**
 * Trim a locator value, uppercase letters and drop leading zeros of numbers.
 */
function ammo_normalize_locator_value($locator, $value) {
  $value = is_scalar($value) ? trim((string) $value) : '';
  if ($locator['type'] === 'letter') {
    return drupal_strtoupper($value);
  }
  if ($locator['type'] === 'numeric' && ctype_digit($value)) {
    return (string) (int) $value;
  }
  return $value;
}

/**
 * The locator values of an amendment (object or row), keyed by locator key.
 */
function ammo_amendment_locator_values($amendment, array $locators) {
  $amendment = (object) $amendment;
  $stored = empty($amendment->locator_values) ? array() : json_decode($amendment->locator_values, TRUE);
  $stored = is_array($stored) ? $stored : array();
  $values = array();
  foreach ($locators as $key => $locator) {
    if (!empty($locator['legacy'])) {
      $values[$key] = isset($amendment->$key) ? (string) $amendment->$key : '';
    }
    else {
      $values[$key] = isset($stored[$key]) ? (string) $stored[$key] : '';
    }
  }
  return $values;
}

/**
 * The database fields that store the submitted locator values of a document.
 */
function ammo_amendment_locator_fields($document, array $input) {
  $locators = ammo_document_locators($document);
  $fields = array();
  $values = array();
  $group = NULL;
  foreach ($locators as $key => $locator) {
    $value = ammo_normalize_locator_value($locator, isset($input[$key]) ? $input[$key] : '');
    if (!empty($locator['legacy'])) {
      $fields[$key] = $value;
      continue;
    }
    if ($value !== '') {
      $values[$key] = $value;
    }
    if (!empty($locator['grouping'])) {
      $group = $value === '' ? NULL : $value;
    }
  }
  $first = reset($locators);
  if (empty($first['legacy'])) {
    // Sorted by key, so reordering the locators does not change stored values.
    ksort($values);
    $fields['locator_values'] = json_encode((object) $values, JSON_UNESCAPED_UNICODE);
    $fields['group_value'] = $group;
  }
  return $fields;
}

/**
 * The names of the database fields that hold the locator values of a document.
 */
function ammo_amendment_locator_columns($document) {
  $locators = ammo_document_locators($document);
  $locator = reset($locators);
  return empty($locator['legacy']) ? array('locator_values', 'group_value') : array_keys($locators);
}

/**
 * A description of the allowed range of a locator, or an empty string.
 */
function ammo_locator_range_description($locator) {
  if ($locator['type'] === 'text') {
    return '';
  }
  if ($locator['min'] !== '' && $locator['max'] !== '') {
    return 'Toegestaan: ' . $locator['min'] . '-' . $locator['max'] . '.';
  }
  if ($locator['min'] !== '') {
    return 'Minimaal: ' . $locator['min'] . '.';
  }
  if ($locator['max'] !== '') {
    return 'Maximaal: ' . $locator['max'] . '.';
  }
  return $locator['type'] === 'letter' ? 'Eén letter (A–Z).' : '';
}

/**
 * Validates locator input against a document; errors are keyed by locator key.
 */
function ammo_validate_locators($document, array $input, array $locators = NULL) {
  $locators = isset($locators) ? $locators : ammo_document_locators($document);
  $errors = array();
  foreach ($locators as $key => $locator) {
    $label = isset($locator['error_label']) ? $locator['error_label'] : $locator['label'];
    $value = ammo_normalize_locator_value($locator, isset($input[$key]) ? $input[$key] : '');
    if ($value === '') {
      if (!empty($locator['required'])) {
        $errors[$key] = $label . ' is verplicht.';
      }
      continue;
    }
    switch ($locator['type']) {
      case 'numeric':
        if (!ctype_digit($value)) {
          $errors[$key] = $label . ' moet een getal zijn.';
        }
        elseif ($locator['min'] !== '' && (int) $value < (int) $locator['min']) {
          $errors[$key] = $label . ' moet minimaal ' . $locator['min'] . ' zijn.';
        }
        elseif ($locator['max'] !== '' && (int) $value > (int) $locator['max']) {
          $errors[$key] = $label . ' mag maximaal ' . $locator['max'] . ' zijn.';
        }
        break;

      case 'letter':
        if (!preg_match('/^[A-Z]$/', $value)) {
          $errors[$key] = $label . ' moet één letter (A–Z) zijn.';
        }
        elseif ($locator['min'] !== '' && strcmp($value, $locator['min']) < 0) {
          $errors[$key] = $label . ' moet minimaal ' . $locator['min'] . ' zijn.';
        }
        elseif ($locator['max'] !== '' && strcmp($value, $locator['max']) > 0) {
          $errors[$key] = $label . ' mag maximaal ' . $locator['max'] . ' zijn.';
        }
        break;

      default:
        if (drupal_strlen($value) > 255) {
          $errors[$key] = $label . ' mag maximaal 255 tekens bevatten.';
        }
    }
  }
  return $errors;
}

/**
 * The SQL expression for the group of an amendment; legacy rows group by chapter.
 */
function ammo_amendment_group_expression($alias = 'a') {
  return 'COALESCE(' . $alias . '.group_value, ' . $alias . '.chapter)';
}

/**
 * The group of an amendment (object or row), or an empty string.
 */
function ammo_amendment_group($amendment) {
  $amendment = (object) $amendment;
  if (isset($amendment->group_value) && $amendment->group_value !== '') {
    return (string) $amendment->group_value;
  }
  return empty($amendment->chapter) ? '' : (string) $amendment->chapter;
}

/**
 * The display number of an amendment, such as 3.12 or B.4.
 */
function ammo_amendment_number($amendment) {
  $amendment = (object) $amendment;
  $group = ammo_amendment_group($amendment);
  return ($group === '' ? 'X' : $group) . '.' . (!empty($amendment->chapterized_id) ? $amendment->chapterized_id : $amendment->id);
}

/**
 * The next sequence number within a group of a document.
 */
function ammo_next_chapterized_id($document_id, $group) {
  $expression = ammo_amendment_group_expression('a');
  return 1 + (int) db_query('SELECT MAX(a.chapterized_id) FROM {ammo_amendments} a WHERE a.document_id = :document AND ' . $expression . ' = :group', array(':document' => $document_id, ':group' => (string) $group))->fetchField();
}

/**
 * The location of an amendment as text, such as "Hoofdstuk 3, pagina 14, regel 220".
 */
function ammo_amendment_location($amendment, array $locators, $include_grouping = TRUE, $inline_first = FALSE) {
  $values = ammo_amendment_locator_values($amendment, $locators);
  $parts = array();
  foreach ($locators as $key => $locator) {
    if ((!$include_grouping && !empty($locator['grouping'])) || $values[$key] === '') {
      continue;
    }
    $label = (empty($parts) && !$inline_first) ? $locator['label'] : ammo_locator_inline_label($locator);
    $parts[] = $label . ' ' . $values[$key];
  }
  return implode(', ', $parts);
}

/**
 * Short location of an amendment in a table: "2 / 14 / 220" for legacy documents, otherwise the full location.
 */
function ammo_amendment_location_short($amendment, $document) {
  $locators = ammo_document_locators($document);
  if (ammo_document_is_legacy(is_object($document) ? $document : entity_load_single('document', $document))) {
    return implode(' / ', array_filter(ammo_amendment_locator_values($amendment, $locators), 'strlen'));
  }
  return ammo_amendment_location($amendment, $locators);
}

/**
 * Compare two values of one locator; empty values sort first.
 */
function ammo_compare_locator_values($type, $a, $b) {
  $a = (string) $a;
  $b = (string) $b;
  if ($a === $b) {
    return 0;
  }
  if ($a === '' || $b === '') {
    return $a === '' ? -1 : 1;
  }
  if ($type === 'numeric' && ctype_digit($a) && ctype_digit($b)) {
    return (int) $a < (int) $b ? -1 : 1;
  }
  return strnatcasecmp($a, $b);
}

/**
 * Sort amendment rows by group and then by location or by number, keeping keys.
 */
function ammo_sort_amendments(array &$rows, array $locators, $by_location = TRUE) {
  $grouping = ammo_grouping_locator($locators);
  $group_type = $grouping ? $grouping['type'] : 'numeric';
  $keys = array();
  foreach ($rows as $id => $row) {
    $keys[$id] = array(ammo_amendment_group($row), ammo_amendment_locator_values($row, $locators));
  }
  uksort($rows, function ($a, $b) use ($rows, $keys, $locators, $group_type, $by_location) {
    $result = ammo_compare_locator_values($group_type, $keys[$a][0], $keys[$b][0]);
    if (!$result && $by_location) {
      foreach ($locators as $key => $locator) {
        if (empty($locator['grouping']) && ($result = ammo_compare_locator_values($locator['type'], $keys[$a][1][$key], $keys[$b][1][$key]))) {
          break;
        }
      }
    }
    if (!$result) {
      $row_a = (object) $rows[$a];
      $row_b = (object) $rows[$b];
      $result = ammo_compare_locator_values('numeric', $row_a->chapterized_id, $row_b->chapterized_id) ?: ammo_compare_locator_values('numeric', $row_a->id, $row_b->id);
    }
    return $result;
  });
}

/**
 * Diff lines for the locators that changed, for the change mails.
 */
function ammo_locator_differences($document, $original, array $fields) {
  $locators = ammo_document_locators($document);
  $old = ammo_amendment_locator_values($original, $locators);
  $new = ammo_amendment_locator_values((object) ($fields + (array) $original), $locators);
  $differences = array();
  foreach ($locators as $key => $locator) {
    if ($old[$key] !== $new[$key]) {
      $differences['locator_' . $key] = array('fieldname' => $locator['label'], 'diff' => $old[$key] . ' → ' . $new[$key]);
    }
  }
  return $differences;
}
