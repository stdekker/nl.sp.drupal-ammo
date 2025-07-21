<?php
function ammo_print_form($form, &$form_state) {
  // Add a wrapper div that will be used by the Form API to update the form using AJAX
  $form['#prefix'] = '<div id="ajax-print-form-wrapper">';
  $form['#suffix'] = '</div>';

  ammo_select_meeting_form($form, $form_state);
  
  // Only add AJAX to meeting field if it's a select dropdown (multiple meetings)
  if (isset($form['meeting']) && $form['meeting']['#type'] == 'select') {
    $form['meeting']['#ajax'] = array(
      'wrapper' => 'ajax-print-form-wrapper',
      'callback' => 'ammo_print_form_ajax_callback',
    );
  }
  
  // If there's only one meeting, automatically set it in form_state
  if (isset($form['meeting']) && $form['meeting']['#type'] == 'value') {
    $form_state['values']['meeting'] = $form['meeting']['#value'];
  }

  if (!empty($form_state['values']['meeting'])) {
    $form['type'] = array(
      '#type' => 'radios',
      '#title' => 'Wat wilt u printen?',
      '#name' => 'type',
      '#required' => TRUE,
      '#options' => array(
        'amendments' => 'wijzigingsvoorstellen',
        'motions' => 'moties',
      ),
      '#ajax' => array(
        'wrapper' => 'ajax-print-form-wrapper',
        'callback' => 'ammo_print_form_ajax_callback',
      ),
    );
  }

  if (!empty($form_state['values']['type'])) {
    switch ($form_state['values']['type']) {
    case 'amendments':
      $form['amendments_order'] = array(
        '#type' => 'radios',
        '#title' => 'Sorteer de wijzigingsvoorstellen op volgorde van',
        '#required' => TRUE,
        '#options' => array(
          'location' => 'de plaats in het document',
          'number' => 'het wijzigingsvoorstel nummer',
        ),
        '#default_value' => 'location',
        '#ajax' => array(
          'wrapper' => 'ajax-print-form-wrapper',
          'callback' => 'ammo_print_form_ajax_callback',
        ),
      );
      unset($form_state['values']['motions_order']);
      break;
    case 'motions':
      $form['motions_order'] = array(
        '#type' => 'radios',
        '#title' => 'Sorteer de moties op volgorde van',
        '#required' => TRUE,
        '#options' => array(
          'branch' => 'de afdeling',
          'number' => 'het nummer van de motie',
        ),
        '#default_value' => 'branch',
        '#ajax' => array(
          'wrapper' => 'ajax-print-form-wrapper',
          'callback' => 'ammo_print_form_ajax_callback',
        ),
      );
      unset($form_state['values']['amendments_order']);
      break;
    }
  }

  if (!empty($form_state['values']['type'])) {
    $form['status'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Statussen'),
      '#options' => ammo_states(),
      '#default_value' => array('accepted', 'submitted', 'rejected'),
      '#required' => TRUE,
    );

    $form['vote_requested_only'] = array(
      '#type' => 'checkbox',
      '#title' => 'Alleen stemming aangevraagd',
      '#default_value' => 0,
    );

    $form['notespace'] = array(
      '#type' => 'checkbox',
      '#title' => 'Ruimte voor notities toevoegen',
    );
    
    $form['pagebreaks'] = array(
      '#type' => 'checkbox',
      '#title' => 'Pagina-einden tussen wijzigingsvoorstellen',
      '#default_value' => 0,
    );
  }

  if (!empty($form_state['values']['type'])) {
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => 'Download print bestand',
      '#name' => 'action_button',
      '#id' => 'action_button',
      '#validate' => array('ammo_print_form_validate'),
      '#submit' => array('ammo_print_form_submit'),
    );
  }

  return $form;
}

function ammo_print_form_validate($form, &$form_state) {
}

function ammo_print_form_submit($form, &$form_state) {
  // Get meeting information to build filename
  $meeting = entity_load_single('meeting', $form_state['values']['meeting']);
  $meeting_name = isset($meeting->title) ? $meeting->title : 'onbekend';
  
  // Get content type name
  $content_type = '';
  switch ($form_state['values']['type']) {
    case 'amendments':
      $content_type = 'wijzigingsvoorstellen';
      break;
    case 'motions':
      $content_type = 'moties';
      break;
  }
  
  // Generate date/time string
  $date_time = date('Y-m-d--H-i');
  
  // Sanitize meeting name for filename
  $sanitized_meeting_name = ammo_sanitize_filename($meeting_name);
  
  // Build filename
  $filename = 'ammo-' . $sanitized_meeting_name . '-' . $content_type . '-' . $date_time . '.docx';
  
  header('Content-type: application/vnd.openxmlformats-officedocument.wordprocessingml.document');
  header('Content-Disposition: attachement; filename="' . $filename . '";');
  // open the "output" stream
  $f = fopen('php://output', 'w');

  switch ($form_state['values']['type']) {
  case 'amendments':
    switch ($form_state['values']['amendments_order']) {
    case 'location':
      $content = theme('print_amendments_by_location', array('values' => $form_state['values']));
      break;
    case 'number':
      $content = theme('print_amendments_by_number', array('values' => $form_state['values']));
      break;
    }
    break;
  case 'motions':
    switch ($form_state['values']['motions_order']) {
    case 'branch':
      $content = theme('print_motions_by_branch', array('values' => $form_state['values']));
      break;
    case 'number':
      $content = theme('print_motions_by_number', array('values' => $form_state['values']));
      break;
    }
    break;
  }

  // UTF-8 encoding is now fully supported by modern Word versions
  // No longer converting to ISO as it causes issues with emoji and special characters

  fputs($f, $content);
  fclose($f);
  drupal_exit();
}

function ammo_print_form_ajax_callback($form, &$form_state) {
  return $form;
}

// Function removed: ammo_encodeToIso() is no longer needed
// Modern Word versions support UTF-8 directly

/**
 * Sanitize a string for use in filenames
 */
function ammo_sanitize_filename($filename) {
  // Convert to lowercase
  $filename = strtolower($filename);
  
  // Replace spaces and special characters with dashes
  $filename = preg_replace('/[^a-z0-9\-]/', '-', $filename);
  
  // Remove multiple consecutive dashes
  $filename = preg_replace('/-+/', '-', $filename);
  
  // Remove leading and trailing dashes
  $filename = trim($filename, '-');
  
  // If empty after sanitization, use 'onbekend'
  if (empty($filename)) {
    $filename = 'onbekend';
  }
  
  return $filename;
}
