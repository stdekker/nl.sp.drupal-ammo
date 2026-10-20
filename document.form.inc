<?php

function ammo_document_list_page() {
  $meetings = entity_load('meeting');
  $documents = entity_load('document');
  $output = '';

  if (empty($meetings)) {
    $output .= '<p>Er zijn geen bijeenkomsten gevonden. Maak eerst <a href="/admin/ammo/meeting/add">een bijeenkomst</a> aan.</p>';
    return $output;
  }

  // Group documents by meeting
  $grouped_documents = array();
  foreach ($documents as $doc) {
    $grouped_documents[$doc->meeting_id][] = $doc;
  }

  foreach ($meetings as $meeting) {
    $output .= '<h2>' . l($meeting->title, 'admin/ammo/meeting/' . $meeting->id . '/edit', array('attributes' => array('title' => 'Bewerk bijeenkomst instellingen'))) . '</h2>';
    if (isset($grouped_documents[$meeting->id]) && !empty($grouped_documents[$meeting->id])) {
      $header = array('Titel', 'Omschrijving', 'Operaties');
      $rows = array();
      foreach ($grouped_documents[$meeting->id] as $doc) {
        $rows[] = array(
          $doc->title,
          $doc->description,
          l('Bewerken', 'admin/ammo/document/' . $doc->id . '/edit') . ' | ' .
          l('Verwijderen', 'admin/ammo/document/' . $doc->id . '/delete'),
        );
      }
      $output .= theme('table', array('header' => $header, 'rows' => $rows));
    } else {
      $output .= '<p>Nog geen documenten voor deze bijeenkomst.</p>';
    }
  }

  return $output;
}


function ammo_document_form($form, &$form_state, $document = NULL) {
  $form = array();

  if ($document) {
    $form['#title'] = 'Bewerk document';
    $form_state['document'] = $document;
  } else {
    $form['#title'] = 'Voeg een document toe';
  }

  if (!$document) {
    $meetings = entity_load('meeting');
    $options = array();
    foreach ($meetings as $meeting) {
      $options[$meeting->id] = $meeting->title;
    }
    if (!empty($options)) {
      if (count($options) == 1) {
        // Only one meeting available, select it automatically
        $meeting_keys = array_keys($options);
        $selected_meeting = reset($meetings);
        $form['meeting_id'] = array(
          '#type' => 'value',
          '#value' => $meeting_keys[0],
        );
        $form['selected_meeting_display'] = array(
          '#type' => 'item',
          '#title' => 'Bijeenkomst',
          '#markup' => check_plain($selected_meeting->title),
        );
      } else {
        // Multiple meetings, show dropdown
        $form['meeting_id'] = array(
          '#type' => 'select',
          '#title' => 'Bijeenkomst',
          '#options' => $options,
          '#required' => TRUE,
          '#empty_option' => 'Kies een bijeenkomst',
        );
      }
    } else {
        drupal_set_message('Er zijn geen bijeenkomsten gevonden. Maak eerst een bijeenkomst aan.', 'warning');
        return $form;
    }
  } else {
      $form['meeting_id'] = array(
        '#type' => 'value',
        '#value' => $document->meeting_id,
      );
  }


  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => 'Titel',
    '#default_value' => $document ? $document->title : '',
    '#required' => TRUE,
  );

  $form['description'] = array(
    '#type' => 'textarea',
    '#title' => 'Omschrijving',
    '#default_value' => $document ? $document->description : '',
  );

  if ($document && ammo_document_is_legacy($document)) {
    // Validation ranges fieldset
    $form['validation_ranges'] = array(
      '#type' => 'fieldset',
      '#title' => 'Validatie instellingen',
      '#description' => 'Stel de toegestane waarden in voor hoofdstukken, pagina\'s en regelnummers bij wijzigingsvoorstellen.',
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
    );

    $form['validation_ranges']['chapter_min'] = array(
      '#type' => 'textfield',
      '#title' => 'Minimum hoofdstuk',
      '#default_value' => $document ? $document->chapter_min : '1',
      '#size' => 10,
      '#maxlength' => 10,
      '#description' => 'Het laagste toegestane hoofdstuknummer bij wijzigingsvoorstellen. Bijvoorbeeld: 1 voor hoofdstuk 1 als minimum.',
    );
  
    $form['validation_ranges']['chapter_max'] = array(
      '#type' => 'textfield',
      '#title' => 'Maximum hoofdstuk',
      '#default_value' => $document ? $document->chapter_max : '',
      '#size' => 10,
      '#maxlength' => 10,
      '#description' => 'Het hoogste toegestane hoofdstuknummer bij wijzigingsvoorstellen. Bijvoorbeeld: 7 voor een bereik van hoofdstuk 1-7.',
    );

    $form['validation_ranges']['page_min'] = array(
      '#type' => 'textfield',
      '#title' => 'Minimum pagina',
      '#default_value' => $document ? $document->page_min : '1',
      '#size' => 10,
      '#maxlength' => 10,
      '#description' => 'Het laagste toegestane paginanummer bij wijzigingsvoorstellen. Bijvoorbeeld: 1 voor pagina 1 als minimum.',
    );
  
    $form['validation_ranges']['page_max'] = array(
      '#type' => 'textfield',
      '#title' => 'Maximum pagina',
      '#default_value' => $document ? $document->page_max : '',
      '#size' => 10,
      '#maxlength' => 10,
      '#description' => 'Het hoogste toegestane paginanummer bij wijzigingsvoorstellen. Bijvoorbeeld: 123 voor een bereik van pagina 1-123.',
    );

    $form['validation_ranges']['line_max'] = array(
      '#type' => 'textfield',
      '#title' => 'Maximum regelnummer',
      '#default_value' => $document ? $document->line_max : '',
      '#size' => 10,
      '#maxlength' => 10,
      '#description' => 'Het hoogste toegestane regelnummer bij wijzigingsvoorstellen. Bijvoorbeeld: 500 als maximum regelnummer per pagina.',
    );
  }
  else {
    ammo_document_locators_form($form, $document);
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Opslaan',
  );

  return $form;
}

function ammo_document_form_validate($form, &$form_state) {
  $values = $form_state['values'];
  
  if (empty($values['title'])) {
    form_set_error('title', 'Vul een titel in.');
  }

  if (isset($values['locators'])) {
    ammo_document_locators_validate($values['locators'], isset($form_state['document']) ? $form_state['document'] : NULL);
    return;
  }
  
  // Validate chapter range
  if (!empty($values['chapter_min']) && !preg_match('/^[0-9]+$/', $values['chapter_min'])) {
    form_set_error('chapter_min', 'Minimum hoofdstuk moet een geldig getal zijn.');
  }
  
  if (!empty($values['chapter_max']) && !preg_match('/^[0-9]+$/', $values['chapter_max'])) {
    form_set_error('chapter_max', 'Maximum hoofdstuk moet een geldig getal zijn.');
  }
  
  if (!empty($values['chapter_min']) && !empty($values['chapter_max'])) {
    if ((int)$values['chapter_min'] > (int)$values['chapter_max']) {
      form_set_error('chapter_max', 'Maximum hoofdstuk moet groter zijn dan minimum hoofdstuk.');
    }
  }
  
  // Validate page range
  if (!empty($values['page_min']) && !preg_match('/^[0-9]+$/', $values['page_min'])) {
    form_set_error('page_min', 'Minimum pagina moet een geldig getal zijn.');
  }
  
  if (!empty($values['page_max']) && !preg_match('/^[0-9]+$/', $values['page_max'])) {
    form_set_error('page_max', 'Maximum pagina moet een geldig getal zijn.');
  }
  
  if (!empty($values['page_min']) && !empty($values['page_max'])) {
    if ((int)$values['page_min'] > (int)$values['page_max']) {
      form_set_error('page_max', 'Maximum pagina moet groter zijn dan minimum pagina.');
    }
  }
  
  // Validate line maximum
  if (!empty($values['line_max']) && !preg_match('/^[0-9]+$/', $values['line_max'])) {
    form_set_error('line_max', 'Maximum regelnummer moet een geldig getal zijn.');
  }
}

function ammo_document_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  $document = isset($form_state['document']) ? $form_state['document'] : NULL;
  
  $entity_values = array(
    'meeting_id' => $values['meeting_id'],
    'title' => $values['title'],
    'description' => $values['description'],
  );
  if (isset($values['locators'])) {
    $entity_values['locators'] = ammo_document_locators_value($values['locators'], $document);
  }
  else {
    $entity_values += array(
      'chapter_min' => !empty($values['chapter_min']) ? (int)$values['chapter_min'] : 1,
      'chapter_max' => !empty($values['chapter_max']) ? (int)$values['chapter_max'] : NULL,
      'page_min' => !empty($values['page_min']) ? (int)$values['page_min'] : 1,
      'page_max' => !empty($values['page_max']) ? (int)$values['page_max'] : NULL,
      'line_max' => !empty($values['line_max']) ? (int)$values['line_max'] : NULL,
    );
  }
  
  $document_id = $document ? $document->id : NULL;

  $saved = ammo_save_entity('document', $entity_values, $document_id);

  if ($saved) {
    drupal_set_message('Document is opgeslagen.');
    $form_state['redirect'] = 'admin/ammo/document';
  } else {
    drupal_set_message('Document is niet opgeslagen.', 'error');
  }
}

/**
 * Adds the locator editor for a document that does not use chapter, page and line.
 */
function ammo_document_locators_form(&$form, $document) {
  $locked = ammo_document_locators_locked($document);
  $form['locators'] = array(
    '#type' => 'fieldset',
    '#title' => 'Locaties',
    '#description' => 'Met locaties geven indieners aan waar hun wijzigingsvoorstel in het document staat. De groeperende locatie is een getal of een letter en bepaalt de indeling en de nummering, zoals 3.12 of B.4. Laat de naam leeg om een locatie te verwijderen.',
    '#tree' => TRUE,
    '#theme' => 'ammo_locators_table',
  );
  if ($locked) {
    $form['locators']['#description'] = 'Er zijn al wijzigingsvoorstellen bij dit document. Je kunt de naam, het minimum en maximum, verplicht en de volgorde nog aanpassen. Locaties toevoegen of verwijderen, het type wijzigen of een andere groeperende locatie kiezen kan niet meer.';
  }
  $rows = array_values($document ? ammo_document_locators($document) : ammo_default_locators());
  if (!$locked) {
    $rows[] = array();
    $rows[] = array();
  }
  foreach ($rows as $delta => $locator) {
    $locator += array('key' => '', 'label' => '', 'type' => 'numeric', 'min' => '', 'max' => '', 'grouping' => FALSE, 'required' => TRUE);
    $form['locators'][$delta] = array(
      'key' => array(
        '#type' => 'value',
        '#value' => $locator['key'],
      ),
      'label' => array(
        '#type' => 'textfield',
        '#title' => 'Naam',
        '#title_display' => 'invisible',
        '#default_value' => $locator['label'],
        '#size' => 20,
        '#maxlength' => 64,
      ),
      'type' => array(
        '#type' => 'select',
        '#title' => 'Type',
        '#title_display' => 'invisible',
        '#options' => ammo_locator_types(),
        '#default_value' => $locator['type'],
        '#disabled' => $locked,
      ),
      'min' => array(
        '#type' => 'textfield',
        '#title' => 'Minimum',
        '#title_display' => 'invisible',
        '#default_value' => $locator['min'],
        '#size' => 5,
        '#maxlength' => 10,
      ),
      'max' => array(
        '#type' => 'textfield',
        '#title' => 'Maximum',
        '#title_display' => 'invisible',
        '#default_value' => $locator['max'],
        '#size' => 5,
        '#maxlength' => 10,
      ),
      'grouping' => array(
        '#type' => 'checkbox',
        '#title' => 'Groeperend',
        '#title_display' => 'invisible',
        '#default_value' => $locator['grouping'],
        '#disabled' => $locked,
      ),
      'required' => array(
        '#type' => 'checkbox',
        '#title' => 'Verplicht',
        '#title_display' => 'invisible',
        '#default_value' => $locator['required'],
      ),
      'weight' => array(
        '#type' => 'weight',
        '#title' => 'Volgorde',
        '#title_display' => 'invisible',
        '#delta' => 20,
        '#default_value' => $delta,
        '#attributes' => array('class' => array('locator-weight')),
      ),
    );
  }
}

/**
 * Renders the locator editor as a sortable table.
 */
function theme_ammo_locators_table($variables) {
  $element = $variables['element'];
  $header = array('Naam', 'Type', 'Minimum', 'Maximum', 'Groeperend', 'Verplicht', 'Volgorde');
  $rows = array();
  foreach (element_children($element) as $delta) {
    $row = array();
    foreach (array('label', 'type', 'min', 'max', 'grouping', 'required', 'weight') as $field) {
      $row[] = drupal_render($element[$delta][$field]);
    }
    $rows[] = array('data' => $row, 'class' => array('draggable'));
  }
  drupal_add_tabledrag('ammo-locators', 'order', 'sibling', 'locator-weight');
  $output = theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('id' => 'ammo-locators')));
  $output .= '<div class="description">Minimum en maximum gelden voor getallen en letters, bijvoorbeeld 1 en 12 of A en F. Laat ze leeg voor geen grens. De groeperende locatie is altijd verplicht.</div>';
  return $output . drupal_render_children($element);
}

/**
 * Validates the submitted locator rows of a document.
 */
function ammo_document_locators_validate(array $rows, $document) {
  $locked = ammo_document_locators_locked($document);
  $stored = ($document && !ammo_document_is_legacy($document)) ? ammo_document_locators($document) : array();
  $types = ammo_locator_types();
  $labels = array();
  $grouping = 0;
  foreach ($rows as $delta => $row) {
    $row += array('key' => '');
    $name = 'locators][' . $delta . '][';
    $label = trim($row['label']);
    if ($label === '') {
      if ($locked && $row['key'] !== '') {
        form_set_error($name . 'label', 'Een locatie kan niet worden verwijderd zolang er wijzigingsvoorstellen bij het document zijn.');
      }
      continue;
    }
    if (isset($labels[drupal_strtolower($label)])) {
      form_set_error($name . 'label', 'Geef elke locatie een eigen naam.');
    }
    $labels[drupal_strtolower($label)] = TRUE;
    if ($locked && isset($stored[$row['key']])) {
      if ($row['type'] !== $stored[$row['key']]['type'] || !empty($row['grouping']) !== $stored[$row['key']]['grouping']) {
        form_set_error($name . 'type', 'Het type en de groepering kunnen niet meer worden gewijzigd.');
      }
    }
    if (!isset($types[$row['type']])) {
      form_set_error($name . 'type', 'Kies een type voor ' . check_plain($label) . '.');
      continue;
    }
    if (!empty($row['grouping'])) {
      $grouping++;
      if ($row['type'] === 'text') {
        form_set_error($name . 'grouping', 'De groeperende locatie moet een getal of een letter zijn.');
      }
    }
    $min = trim($row['min']);
    $max = trim($row['max']);
    if ($row['type'] === 'numeric') {
      foreach (array('min' => $min, 'max' => $max) as $field => $value) {
        if ($value !== '' && !ctype_digit($value)) {
          form_set_error($name . $field, 'Minimum en maximum van ' . check_plain($label) . ' moeten getallen zijn.');
        }
      }
      if (ctype_digit($min) && ctype_digit($max) && (int) $min > (int) $max) {
        form_set_error($name . 'max', 'Het maximum van ' . check_plain($label) . ' moet groter zijn dan het minimum.');
      }
    }
    elseif ($row['type'] === 'letter') {
      foreach (array('min' => $min, 'max' => $max) as $field => $value) {
        if ($value !== '' && !preg_match('/^[A-Za-z]$/', $value)) {
          form_set_error($name . $field, 'Minimum en maximum van ' . check_plain($label) . ' moeten één letter (A–Z) zijn.');
        }
      }
      if ($min !== '' && $max !== '' && strcmp(drupal_strtoupper($min), drupal_strtoupper($max)) > 0) {
        form_set_error($name . 'max', 'Het maximum van ' . check_plain($label) . ' moet na het minimum komen.');
      }
    }
  }
  if (!$labels) {
    form_set_error('locators', 'Voeg minimaal één locatie toe.');
  }
  elseif ($grouping !== 1) {
    form_set_error('locators', 'Kies precies één groeperende locatie.');
  }
}

/**
 * The JSON locator list of the submitted rows, in their chosen order.
 */
function ammo_document_locators_value(array $rows, $document) {
  $stored = ($document && !ammo_document_is_legacy($document)) ? ammo_document_locators($document) : array();
  $locked = ammo_document_locators_locked($document);
  uasort($rows, function ($a, $b) {
    return $a['weight'] - $b['weight'];
  });
  // Keys of removed locators are never reused.
  $next = 1;
  foreach (array_merge(array_keys($stored), array_column($rows, 'key')) as $key) {
    if (preg_match('/^loc(\d+)$/', (string) $key, $match)) {
      $next = max($next, $match[1] + 1);
    }
  }
  $locators = array();
  foreach ($rows as $row) {
    $label = trim($row['label']);
    if ($label === '') {
      continue;
    }
    $key = !empty($row['key']) ? $row['key'] : 'loc' . $next++;
    $type = $row['type'];
    $grouping = !empty($row['grouping']);
    if ($locked && isset($stored[$key])) {
      $type = $stored[$key]['type'];
      $grouping = $stored[$key]['grouping'];
    }
    $range = array();
    foreach (array('min', 'max') as $field) {
      $value = trim($row[$field]);
      if ($type === 'text' || $value === '') {
        $range[$field] = '';
      }
      else {
        $range[$field] = $type === 'letter' ? drupal_strtoupper($value) : (string) (int) $value;
      }
    }
    $locators[] = array(
      'key' => $key,
      'label' => $label,
      'type' => $type,
      'min' => $range['min'],
      'max' => $range['max'],
      'grouping' => $grouping,
      'required' => $grouping || !empty($row['required']),
    );
  }
  return json_encode($locators, JSON_UNESCAPED_UNICODE);
}

function ammo_document_delete_confirm_form($form, &$form_state, $document) {
  $form['#document'] = $document;
  return confirm_form($form,
    'Weet je zeker dat je het document "' . $document->title . '" wilt verwijderen?',
    'admin/ammo/document',
    'Dit verwijdert de ingediende voorstellen en kan niet ongedaan worden gemaakt.',
    'Document definitief verwijderen',
    'Annuleren'
  );
}

function ammo_document_delete_confirm_form_submit($form, &$form_state) {
  $document = $form['#document'];
  entity_delete('document', $document->id);
  drupal_set_message('Document "' . $document->title . '" is verwijderd.');
  $form_state['redirect'] = 'admin/ammo/document';
}
