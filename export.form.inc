<?php
function ammo_export_form($form, &$form_state) {

  $form['intro'] = array(
    '#markup' => '<p>Exporteer de amendementen en moties van een bijeenkomst als xml bestand geschikt voor Adobe InDesign.</p>',
  );

  $options = ammo_get_meetings();
  $form['meeting'] = array(
    '#type' => 'select',
    '#title' => 'Meeting',
    '#options' => $options,
    '#required' => TRUE,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Exporteer',
  );
  return $form;
}

function ammo_export_form_validate($form, &$form_state) {
}

function ammo_export_form_submit($form, &$form_state) {
  // Get meeting information to build filename
  $meeting = entity_load_single('meeting', $form_state['values']['meeting']);
  $meeting_name = isset($meeting->title) ? $meeting->title : 'onbekend';
  
  // Generate date/time string
  $date_time = date('Y-m-d--H-i');
  
  // Sanitize meeting name for filename
  $sanitized_meeting_name = ammo_sanitize_filename($meeting_name);
  
  // Build filename
  $filename = 'ammo-' . $sanitized_meeting_name . '-export-' . $date_time . '.xml';
  
  header('Content-type: text/xml');
  header('Content-Disposition: attachement; filename="' . $filename . '";');
  // open the "output" stream
  $f = fopen('php://output', 'w');

  $content = theme('export_meeting_xml', array('values' => $form_state['values']));

  fputs($f, $content);
  fclose($f);
  drupal_exit();
}

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
