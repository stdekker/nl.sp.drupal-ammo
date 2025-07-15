<?php

/**
 * Form function for amendment vote request management
 */
function ammo_amendment_vote_request_admin_form($form, &$form_state, $amendment_id) {
  return ammo_vote_request_admin_form($form, $form_state, 'amendment', $amendment_id);
}

/**
 * Form function for motion vote request management
 */
function ammo_motion_vote_request_admin_form($form, &$form_state, $motion_id) {
  return ammo_vote_request_admin_form($form, $form_state, 'motion', $motion_id);
}

/**
 * Generic vote request management form for admins
 */
function ammo_vote_request_admin_form($form, &$form_state, $entity_type, $entity_id) {
  // Check if entity exists
  $entity = entity_load_single($entity_type, $entity_id);
  if (empty($entity)) {
    drupal_set_message('Het ' . ($entity_type == 'amendment' ? 'wijzigingsvoorstel' : 'motie') . ' kon niet worden gevonden.', 'error');
    return array();
  }

  // Check admin access
  if (!ammo_admin_access()) {
    drupal_access_denied();
    return array();
  }

  $form_state['storage']['entity_type'] = $entity_type;
  $form_state['storage']['entity_id'] = $entity_id;

  // Page title
  $title = ($entity_type == 'amendment') ? 'Wijzigingsvoorstel' : 'Motie';
  $form['title'] = array(
    '#markup' => '<h2>Stemmingaanvragen beheren - ' . $title . ' #' . $entity_id . '</h2>',
  );

  // Show current proposal info
  $form['current_entity'] = array(
    '#type' => 'fieldset',
    '#title' => 'Huidig ' . strtolower($title),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  $form['current_entity']['entity_display'] = array(
    '#markup' => theme($entity_type, array('entity_id' => $entity_id, 'no_links' => TRUE)),
  );

  // Current vote requests section
  $form['current_requests'] = array(
    '#type' => 'fieldset',
    '#title' => 'Huidige stemmingaanvragen',
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  $requesters = ammo_get_related_contacts($entity_id, $entity_type, 'vote_request', 'branch');

  $current_info = '<div class="current-requests">';
  if (!empty($requesters)) {
    $current_info .= '<p><strong>Afdelingen die stemming hebben aangevraagd:</strong></p><ul>';
    foreach ($requesters as $requester) {
      $current_info .= '<li>' . check_plain($requester['contact_display_name']) . '</li>';
    }
    $current_info .= '</ul>';
  } else {
    $current_info .= '<p><em>Nog geen afdelingen hebben stemming aangevraagd.</em></p>';
  }
  $current_info .= '</div>';

  $form['current_requests']['current_info'] = array(
    '#markup' => $current_info,
  );

  // Change vote requests section
  $form['change_requests'] = array(
    '#type' => 'fieldset',
    '#title' => 'Stemmingaanvragen wijzigen',
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  // Get all afdelingen for selection
  $all_afdelingen = ammo_get_afdelingen('all');
  if (empty($all_afdelingen)) {
    $all_afdelingen = array();
  }

  // Vote requesting branches management
  $form['change_requests']['requesting_branches'] = array(
    '#type' => 'checkboxes',
    '#title' => 'Afdelingen die stemming aanvragen',
    '#description' => 'Selecteer welke afdelingen stemming aanvragen voor dit voorstel.',
    '#options' => $all_afdelingen,
    '#default_value' => isset($form_state['values']['requesting_branches']) ? $form_state['values']['requesting_branches'] : array_keys($requesters),
  );

  // Submit buttons
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Stemmingaanvragen opslaan',
  );

  $dest = drupal_get_destination();
  $form['actions']['cancel'] = array(
    '#markup' => l('Annuleren', $dest['destination'], array('attributes' => array('class' => array('button')))),
  );

  return $form;
}

/**
 * Validation function for vote request admin forms
 */
function ammo_amendment_vote_request_admin_form_validate($form, &$form_state) {
  ammo_vote_request_admin_form_validate($form, $form_state);
}

function ammo_motion_vote_request_admin_form_validate($form, &$form_state) {
  ammo_vote_request_admin_form_validate($form, $form_state);
}

function ammo_vote_request_admin_form_validate($form, &$form_state) {
  // No specific validation needed for now, but placeholder is here.
}

/**
 * Submit function for vote request admin forms
 */
function ammo_amendment_vote_request_admin_form_submit($form, &$form_state) {
  ammo_vote_request_admin_form_submit($form, $form_state);
}

function ammo_motion_vote_request_admin_form_submit($form, &$form_state) {
  ammo_vote_request_admin_form_submit($form, $form_state);
}

function ammo_vote_request_admin_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  $entity_type = $form_state['storage']['entity_type'];
  $entity_id = $form_state['storage']['entity_id'];
  
  $changes_made = FALSE;
  
  // Get current vote requesters
  $current_requesters = ammo_get_related_contacts($entity_id, $entity_type, 'vote_request', 'branch');
  $current_requester_ids = array_keys($current_requesters);
  
  $selected_requesters = array_filter($values['requesting_branches']);
  
  // Find branches to add
  $to_add = array_diff($selected_requesters, $current_requester_ids);
  foreach ($to_add as $branch_id) {
    $afdeling = ammo_get_branch($branch_id);
    if ($afdeling) {
      ammo_save_relation(
        'vote_request',
        $entity_type,
        $entity_id,
        $branch_id,
        'branch',
        $afdeling->title->value(),
        $branch_id,
        $afdeling->title->value()
      );
      $changes_made = TRUE;
    }
  }
  
  // Find branches to remove
  $to_remove = array_diff($current_requester_ids, $selected_requesters);
  foreach ($to_remove as $branch_id) {
    db_delete('ammo_contact_relations')
      ->condition('target_type', $entity_type)
      ->condition('target_id', $entity_id)
      ->condition('relation_type', 'vote_request')
      ->condition('contact_type', 'branch')
      ->condition('contact_id', $branch_id)
      ->execute();
    $changes_made = TRUE;
  }
  
  if ($changes_made) {
    $type_name = ($entity_type == 'amendment') ? 'wijzigingsvoorstel' : 'motie';
    drupal_set_message('De stemmingaanvragen voor ' . $type_name . ' #' . $entity_id . ' zijn bijgewerkt.');
  } else {
    drupal_set_message('Geen wijzigingen aangebracht.');
  }
  
  // Redirect back to where we came from
  $dest = drupal_get_destination();
  if (!empty($dest['destination'])) {
      $form_state['redirect'] = $dest['destination'];
  } else {
      $form_state['redirect'] = $entity_type . '/' . $entity_id;
  }
} 