<?php

function ammo_meeting_list_page() {
  $meetings = entity_load('meeting');
  $output = '';

  if (empty($meetings)) {
    $output .= '<p>Er zijn geen bijeenkomsten gevonden.</p>';
    return $output;
  }

  $header = array(
    'Titel', 
    'Beschrijving', 
    'Datum', 
    'Contact e-mail', 
    'Termijnen',
    'Instellingen',
    'Operaties'
  );
  
  $rows = array();
  foreach ($meetings as $meeting) {
    $date = new DateTime($meeting->date);
    $settings = array();
    
    if ($meeting->hide_state) {
      $settings[] = 'Status verborgen';
    }
    if ($meeting->hide_advice) {
      $settings[] = 'Advies verborgen';
    }
    if ($meeting->hide_meeting) {
      $settings[] = 'Bijeenkomst verborgen';
    }
    if (!ammo_meeting_has_motions($meeting)) {
      $settings[] = 'Geen moties';
    }
    if ($meeting->disable_confirmation_checkbox) {
      $settings[] = 'Bevestiging uitgeschakeld';
    }
    
    $settings_text = !empty($settings) ? implode(', ', $settings) : 'Standaard instellingen';
    
    $rows[] = array(
      check_plain($meeting->title),
      check_plain($meeting->description),
      $date->format('d-m-Y'),
      check_plain($meeting->email),
      ammo_meeting_ranges_summary($meeting),
      $settings_text,
      l('Bewerken', 'admin/ammo/meeting/' . $meeting->id . '/edit') . ' | ' .
      l('Testgegevens', 'admin/ammo/meeting/' . $meeting->id . '/test-data') . ' | ' .
      l('Verwijderen', 'admin/ammo/meeting/' . $meeting->id . '/delete'),
    );
  }
  
  $output .= theme('table', array('header' => $header, 'rows' => $rows));
  return $output;
}

function ammo_meeting_form($form, &$form_state) {
  $form = array();

  $form['#title'] = 'Voeg een bijeenkomst toe';

  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => 'Titel',
  );

  $form['description'] = array(
    '#type' => 'textarea',
    '#title' => 'Beschrijving',
  );

  $form['email'] = array(
    '#type' => 'textfield',
    '#title' => 'Contact e-mailadres',
  );

  $form['date'] = array(
    '#title' => 'Datum evenement',
    '#type' => 'date_popup',
    '#date_format' => 'd-m-Y',
  );

  ammo_meeting_ranges_form($form);

  $form['hide_state'] = array(
    '#type' => 'checkbox',
    '#title' => 'Verberg status.',
  );

  $form['hide_advice'] = array(
    '#type' => 'checkbox',
    '#title' => 'Verberg advies.',
  );

  $form['hide_meeting'] = array(
    '#type' => 'checkbox',
    '#title' => 'Verberg bijeenkomst.',
  );

  $form['disable_confirmation_checkbox'] = array(
    '#type' => 'checkbox',
    '#title' => 'Uitschakelen bevestiging wijzigingsvoorstellen.',
    '#description' => 'Schakel de bevestiging "Ik heb gecontroleerd dat er niet al een vergelijkbaar wijzigingsvoorstel is ingediend." uit bij het start van het indienen van wijzigingsvoorstellen.',
    '#default_value' => 0,
  );

  $form['admin_owner'] = array(
    '#type' => 'textfield',
    '#title' => 'Indienen amendementen, moties en wijzigingen door beheerders namens',
    '#default_value' => 'de congrescommissie',
    '#required' => TRUE,
  );
  if (!empty($form_state['values']['admin_owner'])) {
    $form['admin_owner']['#disabled'] = TRUE;
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Bijeenkomst toevoegen',
  );
  return $form;
}

function ammo_meeting_form_validate($form, &$form_state) {
  $values = $form_state['values'];
  
  if (empty($values['title'])) {
    form_set_error('title', 'Vul een titel in.');
  }
  if (empty($values['date'])) {
    form_set_error('date', 'Kies een datum.');
  }
  ammo_meeting_ranges_validate($values);
  if (empty($values['withdrawal_closing_date'])) {
    form_set_error('withdrawal_closing_date', 'Kies een laatste intrekdatum.');
  }
  if (empty($values['email']) || !valid_email_address($values['email'])) {
    form_set_error('email', 'Vul een geldig e-mailadres in.');
  }
  
  // Check if title is unique for new meeting
  $unique_title = ammo_entity_check_title('meeting', $values['title']);
  if ($unique_title === FALSE) {
    form_set_error('title', 'Titel is reeds in gebruik.');
  }
}

function ammo_meeting_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  
  $entity_values = array(
    'title' => $values['title'],
    'description' => $values['description'],
    'date' => $values['date'],
    'withdrawal_closing_date' => ammo_meeting_end_of_day($values['withdrawal_closing_date']),
    'email' => $values['email'],
    'no_member_owners' => 1,
    'hide_state' => $values['hide_state'],
    'hide_advice' => $values['hide_advice'],
    'hide_meeting' => $values['hide_meeting'],
    'disable_confirmation_checkbox' => $values['disable_confirmation_checkbox'],
    'admin_owner' => $values['admin_owner'],
  ) + ammo_meeting_ranges_values($values);
  
  $saved = ammo_save_entity('meeting', $entity_values, NULL);
  
  if ($saved) {
    drupal_set_message('Bijeenkomst is aangemaakt.');
    $form_state['redirect'] = 'admin/ammo/meeting';
  } else {
    drupal_set_message('Bijeenkomst is niet opgeslagen.', 'error');
  }
}

function ammo_meeting_form_ajax($form, &$form_state) {
  return $form;
}

function ammo_meeting_edit_form($form, &$form_state, $meeting) {
  $form = array();
  
  $form['#title'] = 'Bewerk bijeenkomst: ' . check_plain($meeting->title);
  $form_state['meeting'] = $meeting;

  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => 'Titel',
    '#default_value' => $meeting->title,
    '#required' => TRUE,
  );

  $form['description'] = array(
    '#type' => 'textarea',
    '#title' => 'Beschrijving',
    '#default_value' => $meeting->description,
  );

  $form['email'] = array(
    '#type' => 'textfield',
    '#title' => 'Contact e-mailadres',
    '#default_value' => $meeting->email,
    '#required' => TRUE,
  );

  $date = new DateTime($meeting->date);
  $form['date'] = array(
    '#title' => 'Datum evenement',
    '#type' => 'date_popup',
    '#date_format' => 'd-m-Y',
    '#default_value' => $date->format('Y-m-d'),
    '#required' => TRUE,
  );

  ammo_meeting_ranges_form($form, $meeting);

  $form['hide_state'] = array(
    '#type' => 'checkbox',
    '#title' => 'Verberg status.',
    '#default_value' => $meeting->hide_state,
  );

  $form['hide_advice'] = array(
    '#type' => 'checkbox',
    '#title' => 'Verberg advies.',
    '#default_value' => $meeting->hide_advice,
  );

  $form['hide_meeting'] = array(
    '#type' => 'checkbox',
    '#title' => 'Verberg bijeenkomst.',
    '#default_value' => $meeting->hide_meeting,
  );

  $form['disable_confirmation_checkbox'] = array(
    '#type' => 'checkbox',
    '#title' => 'Uitschakelen bevestiging wijzigingsvoorstellen.',
    '#description' => 'Schakel de bevestiging "Ik heb gecontroleerd dat er niet al een vergelijkbaar wijzigingsvoorstel is ingediend." uit bij het start van het indienen van wijzigingsvoorstellen.',
    '#default_value' => $meeting->disable_confirmation_checkbox,
  );

  $form['admin_owner'] = array(
    '#type' => 'textfield',
    '#title' => 'Indienen amendementen, moties en wijzigingen door beheerders namens',
    '#default_value' => $meeting->admin_owner,
    '#required' => TRUE,
    '#disabled' => TRUE,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Opslaan',
  );

  $form['delete'] = array(
    '#type' => 'submit',
    '#value' => 'Verwijderen',
    '#submit' => array('ammo_meeting_edit_form_delete_submit'),
    '#limit_validation_errors' => array(),
    '#attributes' => array(
      'class' => array('button-danger'),
    ),
  );

  return $form;
}

function ammo_meeting_edit_form_validate($form, &$form_state) {
  $values = $form_state['values'];
  
  if (empty($values['title'])) {
    form_set_error('title', 'Vul een titel in.');
  }
  if (empty($values['date'])) {
    form_set_error('date', 'Kies een datum.');
  }
  ammo_meeting_ranges_validate($values);
  if (empty($values['withdrawal_closing_date'])) {
    form_set_error('withdrawal_closing_date', 'Kies een laatste intrekdatum.');
  }
  if (empty($values['email']) || !valid_email_address($values['email'])) {
    form_set_error('email', 'Vul een geldig e-mailadres in.');
  }
}

function ammo_meeting_edit_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  $meeting = $form_state['meeting'];
  
  $entity_values = array(
    'title' => $values['title'],
    'description' => $values['description'],
    'date' => $values['date'],
    'withdrawal_closing_date' => ammo_meeting_end_of_day($values['withdrawal_closing_date']),
    'email' => $values['email'],
    'no_member_owners' => 1,
    'hide_state' => $values['hide_state'],
    'hide_advice' => $values['hide_advice'],
    'hide_meeting' => $values['hide_meeting'],
    'disable_confirmation_checkbox' => $values['disable_confirmation_checkbox'],
    'admin_owner' => $values['admin_owner'],
  ) + ammo_meeting_ranges_values($values);

  $saved = ammo_save_entity('meeting', $entity_values, $meeting->id);

  if ($saved) {
    drupal_set_message('Bijeenkomst is opgeslagen.');
    $form_state['redirect'] = 'admin/ammo/meeting';
  } else {
    drupal_set_message('Bijeenkomst is niet opgeslagen.', 'error');
  }
}

function ammo_meeting_delete_confirm_form($form, &$form_state, $meeting) {
  $form['#meeting'] = $meeting;
  return confirm_form($form,
    'Bijeenkomst "' . $meeting->title . '" verwijderen',
    'admin/ammo/meeting',
    'Let op! Weet je zeker dat je de bijeenkomst "' . $meeting->title . '" wilt verwijderen? Je bent dan ook de onderliggende documenten en voorstellen kwijt! Dit kan niet ongedaan worden gemaakt.',
    'Bijeenkomst definitief verwijderen',
    'Annuleren'
  );
}

function ammo_meeting_delete_confirm_form_submit($form, &$form_state) {
  $meeting = $form['#meeting'];
  entity_delete('meeting', $meeting->id);
  drupal_set_message('Bijeenkomst "' . $meeting->title . '" is verwijderd.');
  $form_state['redirect'] = 'admin/ammo/meeting';
}

function ammo_meeting_edit_form_delete_submit($form, &$form_state) {
  $meeting = $form_state['meeting'];
  $form_state['redirect'] = 'admin/ammo/meeting/' . $meeting->id . '/delete';
}

/**
 * Adds the Termijnen fieldset with a Van and Tot date for each range of the meeting.
 */
function ammo_meeting_ranges_form(&$form, $meeting = NULL) {
  $form['ranges'] = array(
    '#type' => 'fieldset',
    '#title' => 'Termijnen',
    '#description' => 'Een termijn loopt van het begin van de Van-dag (00:00) tot het einde van de Tot-dag (23:59). Zonder Van is de termijn direct open. Beheerders zijn niet aan de termijnen gebonden.',
  );
  // Amendments and motions side by side, each with its submission and change range.
  $form['ranges']['columns'] = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('ammo-meeting-ranges')),
  );
  $columns = array('amendment' => 'Wijzigingsvoorstellen', 'motion' => 'Moties');
  $short = array('submission' => 'Indienen', 'change' => 'Wijzigen na indienen');
  foreach ($columns as $type => $heading) {
    $column = &$form['ranges']['columns'][$type];
    $column = array(
      '#type' => 'container',
      '#attributes' => array('class' => array('ammo-meeting-ranges-column')),
      'heading' => array('#markup' => '<h3>' . $heading . '</h3>'),
    );
    foreach ($short as $kind => $label) {
      $range = $type . '_' . $kind;
      $definition = ammo_meeting_ranges()[$range];
      $column[$range] = array(
        '#type' => 'container',
        '#attributes' => array('class' => array('ammo-meeting-range')),
        'label' => array('#markup' => '<h4>' . $label . '</h4>'),
      );
      foreach (array('_start' => 'Van', '_end' => 'Tot') as $part => $title) {
        // An empty date stays empty: new DateTime(NULL) would fill in the current time.
        $date = $meeting ? ammo_meeting_range_date($meeting, $range . $part) : NULL;
        $column[$range][$range . $part] = array(
          '#type' => 'date_popup',
          '#title' => $title,
          '#date_format' => 'd-m-Y',
          '#date_label_position' => 'none',
          '#default_value' => $date ? substr($date, 0, 10) : NULL,
        );
      }
      $column[$range]['description'] = array('#markup' => '<div class="description">' . check_plain($definition['empty']) . '</div>');
    }
    unset($column);
  }

  $withdrawal = $meeting ? ammo_meeting_range_date($meeting, 'withdrawal_closing_date') : NULL;
  $form['ranges']['withdrawal_closing_date'] = array(
    '#title' => 'Uiterste intrek/ondersteuningsdatum',
    '#description' => 'Tot en met deze dag (23:59).',
    '#type' => 'date_popup',
    '#date_format' => 'd-m-Y',
    '#date_label_position' => 'none',
    '#default_value' => $withdrawal ? substr($withdrawal, 0, 10) : NULL,
    '#required' => !empty($meeting),
  );
}

/**
 * A form date as the end of its day, or NULL when it is empty.
 */
function ammo_meeting_end_of_day($value) {
  return empty($value) ? NULL : substr($value, 0, 10) . ' 23:59:59';
}

/**
 * Validates the ranges: Tot is needed with Van or for a required range, and Van is not after Tot.
 */
function ammo_meeting_ranges_validate(array $values) {
  foreach (ammo_meeting_ranges() as $range => $definition) {
    $start = isset($values[$range . '_start']) ? $values[$range . '_start'] : '';
    $end = isset($values[$range . '_end']) ? $values[$range . '_end'] : '';
    // An unreadable date stays an array, and date_popup has already reported it.
    if (!is_string($start) || !is_string($end)) {
      continue;
    }
    if (empty($end)) {
      if (!empty($start)) {
        form_set_error($range . '_end', check_plain($definition['label']) . ': kies ook een Tot-datum, of maak Van leeg.');
      }
      elseif ($definition['required']) {
        form_set_error($range . '_end', check_plain($definition['label']) . ': kies een Tot-datum.');
      }
    }
    elseif (!empty($start) && substr($start, 0, 10) > substr($end, 0, 10)) {
      form_set_error($range . '_start', check_plain($definition['label']) . ': Van mag niet na Tot liggen.');
    }
  }
}

/**
 * The range values of the meeting form as entity values: Van from 00:00, Tot until 23:59:59, and NULL for an empty date.
 */
function ammo_meeting_ranges_values(array $values) {
  $entity_values = array();
  foreach (array_keys(ammo_meeting_ranges()) as $range) {
    $start = isset($values[$range . '_start']) ? $values[$range . '_start'] : '';
    $entity_values[$range . '_start'] = empty($start) ? NULL : substr($start, 0, 10) . ' 00:00:00';
    $entity_values[$range . '_end'] = ammo_meeting_end_of_day(isset($values[$range . '_end']) ? $values[$range . '_end'] : '');
  }
  return $entity_values;
}

/**
 * The ranges of a meeting for the meeting list, one line per range.
 */
function ammo_meeting_ranges_summary($meeting) {
  $lines = array();
  foreach (ammo_meeting_ranges() as $range => $definition) {
    $start = ammo_meeting_range_date($meeting, $range . '_start');
    $end = ammo_meeting_range_date($meeting, $range . '_end');
    $text = $end ? ($start ? date('d-m-Y', strtotime($start)) : 'direct') . ' – ' . date('d-m-Y', strtotime($end)) : 'niet';
    $lines[] = check_plain($definition['label'] . ': ' . $text);
  }
  return implode('<br />', $lines);
}
