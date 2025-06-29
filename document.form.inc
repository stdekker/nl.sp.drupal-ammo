<?php

function ammo_document_list_page() {
  $meetings = entity_load('meeting');
  $documents = entity_load('document');
  $output = '';

  if (empty($meetings)) {
    $output .= '<p>Er zijn geen bijeenkomsten gevonden. Maak eerst ' . l('een bijeenkomst', 'admin/ammo/meeting/add') . ' aan.</p>';
    print $output;
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
}

function ammo_document_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  $document = isset($form_state['document']) ? $form_state['document'] : NULL;
  
  $entity_values = array(
    'meeting_id' => $values['meeting_id'],
    'title' => $values['title'],
    'description' => $values['description'],
  );
  
  $document_id = $document ? $document->id : NULL;

  $saved = ammo_save_entity('document', $entity_values, $document_id);

  if ($saved) {
    drupal_set_message('Document is opgeslagen.');
    $form_state['redirect'] = 'admin/ammo/document';
  } else {
    drupal_set_message('Document is niet opgeslagen.', 'error');
  }
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
