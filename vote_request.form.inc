<?php

function ammo_vote_request_form($form, &$form_state, $action, $contact_type, $target_type = NULL, $entity_id = NULL) {
  global $user;
  $user_wrapper = entity_metadata_wrapper('user', $user);
  
  // Fallback for when OpenID is disabled - allow admins to test
  $contact_id = NULL;
  try {
    if (isset($user_wrapper->field_contact_id)) {
      $contact_id = $user_wrapper->field_contact_id->value();
    }
  } catch (EntityMetadataWrapperException $e) {
    // Field doesn't exist, use fallback
  }
  
  // If no contact_id and user is admin, use user ID as fallback
  if (empty($contact_id) && ammo_admin_access()) {
    $contact_id = $user->uid;
  }

  $meeting_id = ammo_get_meeting_id($target_type, $entity_id);
  
  // Get entity to check status
  $entity = entity_load_single($target_type, $entity_id);
  
  if (
    !in_array($action, array('add', 'withdraw')) ||
    !in_array($contact_type, array('branch')) ||
    !in_array($target_type, array('amendment', 'motion')) ||
    filter_var($entity_id, FILTER_VALIDATE_INT) === FALSE
  ) {
    form_set_error('vote_request', 'De pagina kan niet worden geladen.');
    return array();
  }
  
  // Check if vote requests are allowed for this status
  if (!ammo_vote_request_access($meeting_id, $entity->state)) {
    // Provide more specific error messages
    $allowed_statuses = variable_get('ammo_vote_request_allowed_statuses', array('accepted', 'rejected'));
    $states = ammo_states();
    
    if (!in_array($entity->state, $allowed_statuses)) {
      form_set_error('vote_request', 'Het is niet mogelijk stemming aan te vragen voor voorstellen met de status "' . $entity->state . '". Toegestane statussen: ' . implode(', ', array_intersect_key($states, array_flip($allowed_statuses))) . '.');
    } elseif (ammo_passed_date($meeting_id, 'withdraw') && !ammo_admin_access()) {
      form_set_error('vote_request', 'Het is niet meer mogelijk stemming aan te vragen omdat de intrekdatum voor dit evenement is verstreken.');
    } else {
      form_set_error('vote_request', 'Het is niet mogelijk stemming aan te vragen voor dit voorstel.');
    }
    return array();
  }
  
  $form['item'] = array(
    '#type' => 'item',
    '#markup' => theme($target_type, array('entity_id' => $entity_id, 'no_links' => TRUE)),
  );

  switch ($contact_type) {
  case 'branch':
    // Controleer toegang op basis evenement.
    if (!$meeting_id || !ammo_withdraw_access($meeting_id)) {
      form_set_error('vote_request', 'Het is niet (meer) mogelijk stemming aan te vragen.');
      return array();
    }
    // Create texts.
    switch ($action) {
    case 'add':
      switch ($target_type) {
      case 'amendment':
        $text = 'De geselecteerde afdelingen vragen stemming aan voor dit wijzigingsvoorstel.';
        break;
      case 'motion':
        $text = 'De geselecteerde afdelingen vragen stemming aan voor deze motie.';
        break;
      }
      break;
    case 'withdraw':
      switch ($target_type) {
      case 'amendment':
        $text = 'De geselecteerde afdelingen trekken de stemming aanvraag in voor dit wijzigingsvoorstel.';
        break;
      case 'motion':
        $text = 'De geselecteerde afdelingen trekken de stemming aanvraag in voor deze motie.';
        break;
      }
      break;
    }
    // Get afdelingen.
    $branches = ammo_get_afdelingen($contact_id);
    // Filter branches that already requested vote.
    foreach ($branches as $contact_id => $branch) {
      $vote_requested = ammo_is_related($entity_id, $target_type, 'vote_request', $contact_type, $contact_id);
      switch ($action) {
      case 'add':
        if (empty($vote_requested)) {
          $options[$contact_id] = $branch;
        }
        break;
      case 'withdraw':
        if (!empty($vote_requested)) {
          $options[$contact_id] = $branch;
        }
        break;
      }
    }
    if (empty($options)) {
      form_set_error('vote_request', 'Er zijn geen afdelingen gevonden die aan de voorwaarden voldoen.');
      return array();
    }
    $form['branch_id'] = array(
      '#title' => $text,
      '#type' => 'checkboxes',
      '#options' => $options,
    );
    break;
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Bevestig',
    '#submit' => array('ammo_vote_request_form_submit'),
    '#validate' => array('ammo_vote_request_form_validate'),
  );
  return $form;
}

function ammo_vote_request_form_validate($form, &$form_state) {
  global $user;
  $values = $form_state['values'];
  $action = arg(2);
  $contact_type = arg(3);
  $target_type = arg(4);
  $entity_id = arg(5);
  $entity = entity_load_single($target_type, $entity_id);
  
  switch ($contact_type) {
  case 'branch':
    $selected = FALSE;
    foreach ($values['branch_id'] as $key => $value) {
      if ($key == $value) {
        $selected = TRUE;
        $vote_requested = ammo_is_related($entity_id, $target_type, 'vote_request', $contact_type, $value);
        switch ($action) {
        case 'add':
          if ($vote_requested) {
            form_set_error('vote_request', 'Stemming aanvraag was al opgeslagen.');
          }
          break;
        case 'withdraw':
          if (!$vote_requested) {
            form_set_error('vote_request', 'Stemming aanvraag was reeds verwijderd.');
          }
          break;
        }
      }
    }
    if (!$selected) {
      form_set_error('contact_id', 'Vink minimaal één afdeling aan.');
    }
    break;
  }
}

function ammo_vote_request_form_submit($form, &$form_state) {
  global $user;
  $user_wrapper = entity_metadata_wrapper('user', $user);
  
  // Fallback for when OpenID is disabled - allow admins to test
  $contact_id = NULL;
  try {
    if (isset($user_wrapper->field_contact_id)) {
      $contact_id = $user_wrapper->field_contact_id->value();
    }
  } catch (EntityMetadataWrapperException $e) {
    // Field doesn't exist, use fallback
  }
  
  // If no contact_id and user is admin, use user ID as fallback
  if (empty($contact_id) && ammo_admin_access()) {
    $contact_id = $user->uid;
  }

  $values = $form_state['values'];
  $action = arg(2);
  $contact_type = arg(3);
  $target_type = arg(4);
  $entity_id = arg(5);
  $entity = entity_load_single($target_type, $entity_id);
  $afdelingen = ammo_get_afdelingen('all');

  switch ($contact_type) {
  case 'branch':
    switch ($action) {
    case 'add':
      $meeting_id = ammo_get_meeting_id($target_type, $entity_id);
      foreach ($values['branch_id'] as $key => $value) {
        if ($key == $value) {
          $display_name = $afdelingen[$value];
          // Sla nieuwe vote request op.
          $vote_request = ammo_save_relation('vote_request', $target_type, $entity_id, $value, $contact_type, $display_name, $value, $display_name);
          if ($vote_request) {
            drupal_set_message('Stemming aanvraag is opgeslagen.');
            ammo_vote_request_mail($target_type, $entity_id, $vote_request, $meeting_id);
          }
          else {
            drupal_set_message('Er is een fout opgetreden.' , 'error');
          }
        }
      }
      break;
    case 'withdraw':
      $meeting_id = ammo_get_meeting_id($target_type, $entity_id);
      foreach ($values['branch_id'] as $key => $value) {
        if ($key == $value) {
          $remove_id = ammo_is_related($entity_id, $target_type, 'vote_request', $contact_type, $value);
          $vote_request = entity_load_single('contact_relation', $remove_id);
          $result = entity_delete('contact_relation', $remove_id);
          if ($result !== FALSE) {
            drupal_set_message('Stemming aanvraag is ingetrokken.');
            ammo_vote_request_withdraw_mail($target_type, $entity_id, $vote_request, $meeting_id);
          }
          else {
            drupal_set_message('Er is een fout opgetreden.' , 'error');
          }
        }
      }
      break;
    }
    break;
  }
  $form_state['redirect'] = ammo_get_destination();
} 