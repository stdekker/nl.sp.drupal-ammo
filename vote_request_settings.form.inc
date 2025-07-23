<?php

/**
 * Admin form for configuring vote request settings
 */
function ammo_vote_request_settings_form($form, &$form_state) {
  $form['#title'] = 'Stemmingaanvraag instellingen';
  
  $form['description'] = array(
    '#markup' => '<p>Configureer voor welke statussen gebruikers stemming kunnen aanvragen op voorstellen.</p>',
  );
  
  // Get current settings or use defaults
  $current_settings = variable_get('ammo_vote_request_allowed_statuses', array('accepted', 'rejected'));
  
  $form['ammo_vote_request_allowed_statuses'] = array(
    '#type' => 'checkboxes',
    '#title' => 'Toegestane statussen voor stemmingaanvragen',
    '#description' => 'Selecteer de statussen waarbij gebruikers stemming kunnen aanvragen voor wijzigingsvoorstellen en moties.',
    '#options' => ammo_states(),
    '#default_value' => $current_settings,
    '#required' => TRUE,
  );
  
  $form['help_text'] = array(
    '#markup' => '<div class="form-item"><strong>Huidige standaard instellingen:</strong><ul><li><strong>Overgenomen</strong> - Voorstellen die zijn overgenomen door de commissie</li><li><strong>Afgewezen</strong> - Voorstellen die zijn afgewezen door de commissie</li></ul><p>Let op: Als geen statussen zijn geselecteerd, kunnen er geen stemmingaanvragen worden gedaan.</p></div>',
  );
  
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Instellingen opslaan',
  );
  
  return $form;
}

/**
 * Validation function for vote request settings form
 */
function ammo_vote_request_settings_form_validate($form, &$form_state) {
  $values = $form_state['values'];
  
  // Check if at least one status is selected
  $selected_statuses = array_filter($values['ammo_vote_request_allowed_statuses']);
  if (empty($selected_statuses)) {
    form_set_error('ammo_vote_request_allowed_statuses', 'Selecteer minimaal één status waarbij stemmingaanvragen mogelijk zijn.');
  }
  
  // Warn if concept is selected (usually not recommended)
  if (!empty($selected_statuses['concept'])) {
    drupal_set_message('Let op: "Concept" status geselecteerd. Dit betekent dat gebruikers stemming kunnen aanvragen voor conceptvoorstellen.', 'warning');
  }
}

/**
 * Submit function for vote request settings form
 */
function ammo_vote_request_settings_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  
  // Filter out unchecked values and save only the selected statuses
  $selected_statuses = array_filter($values['ammo_vote_request_allowed_statuses']);
  $selected_statuses = array_keys($selected_statuses);
  
  // Save the configuration
  variable_set('ammo_vote_request_allowed_statuses', $selected_statuses);
  
  // Show success message with selected statuses
  $status_labels = ammo_states();
  $selected_labels = array();
  foreach ($selected_statuses as $status) {
    $selected_labels[] = $status_labels[$status];
  }
  
  drupal_set_message('Stemmingaanvraag instellingen zijn opgeslagen. Stemmingaanvragen zijn nu mogelijk voor de volgende statussen: ' . implode(', ', $selected_labels) . '.');
} 