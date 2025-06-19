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
    if ($meeting->no_motions) {
      $settings[] = 'Geen moties';
    }
    
    $settings_text = !empty($settings) ? implode(', ', $settings) : 'Standaard instellingen';
    
    $rows[] = array(
      check_plain($meeting->title),
      check_plain($meeting->description),
      $date->format('d-m-Y'),
      check_plain($meeting->email),
      $settings_text,
      l('Bewerken', 'admin/ammo/meeting/' . $meeting->id . '/edit') . ' | ' .
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

  $form['submission_closing_date'] = array(
    '#title' => 'Uiterste inlever/wijzigingsdatum',
    '#description' => 'Na deze datum kunnen er geen nieuwe wijzigingsvoorstellen en moties meer worden ingeleverd, en kunnen bestaande wijzigingsvoorstellen en moties niet meer worden gewijzigd.',
    '#type' => 'date_popup',
    '#date_format' => 'd-m-Y H:i',
  );

  $form['withdrawal_closing_date'] = array(
    '#title' => 'Uiterste intrek/ondersteuningsdatum',
    '#type' => 'date_popup',
    '#date_format' => 'd-m-Y H:i',
  );

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

  $form['no_motions'] = array(
    '#type' => 'checkbox',
    '#title' => 'Geen moties.',
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
  if (empty($values['submission_closing_date'])) {
    form_set_error('submission_closing_date', 'Kies een laatste inleverdatum.');
  }
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
    'submission_closing_date' => $values['submission_closing_date'],
    'withdrawal_closing_date' => $values['withdrawal_closing_date'],
    'email' => $values['email'],
    'no_member_owners' => 1,
    'hide_state' => $values['hide_state'],
    'hide_advice' => $values['hide_advice'],
    'hide_meeting' => $values['hide_meeting'],
    'no_motions' => $values['no_motions'],
    'admin_owner' => $values['admin_owner'],
  );
  
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

  $submission_date = new DateTime($meeting->submission_closing_date);
  $form['submission_closing_date'] = array(
    '#title' => 'Uiterste inlever/wijzigingsdatum',
    '#description' => 'Na deze datum kunnen er geen nieuwe wijzigingsvoorstellen en moties meer worden ingeleverd, en kunnen bestaande wijzigingsvoorstellen en moties niet meer worden gewijzigd.',
    '#type' => 'date_popup',
    '#date_format' => 'd-m-Y H:i',
    '#default_value' => $submission_date->format('Y-m-d H:i'),
    '#required' => TRUE,
  );

  $withdrawal_date = new DateTime($meeting->withdrawal_closing_date);
  $form['withdrawal_closing_date'] = array(
    '#title' => 'Uiterste intrek/ondersteuningsdatum',
    '#type' => 'date_popup',
    '#date_format' => 'd-m-Y H:i',
    '#default_value' => $withdrawal_date->format('Y-m-d H:i'),
    '#required' => TRUE,
  );

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

  $form['no_motions'] = array(
    '#type' => 'checkbox',
    '#title' => 'Geen moties.',
    '#default_value' => $meeting->no_motions,
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
  if (empty($values['submission_closing_date'])) {
    form_set_error('submission_closing_date', 'Kies een laatste inleverdatum.');
  }
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
    'submission_closing_date' => $values['submission_closing_date'],
    'withdrawal_closing_date' => $values['withdrawal_closing_date'],
    'email' => $values['email'],
    'no_member_owners' => 1,
    'hide_state' => $values['hide_state'],
    'hide_advice' => $values['hide_advice'],
    'hide_meeting' => $values['hide_meeting'],
    'no_motions' => $values['no_motions'],
    'admin_owner' => $values['admin_owner'],
  );

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
