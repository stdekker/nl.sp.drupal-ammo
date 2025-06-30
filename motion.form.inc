<?php

function ammo_motion_form($form, &$form_state, $form_action, $path_id = NULL)
{
  global $user;

  // Store destination before ajax call.
  ammo_set_destination($form_state);

  // Check if motion is set in path and preset form if so.
  if (!empty($path_id) && !empty($form_action) && empty($form_state['storage']['first_step'])) {
    switch ($form_action) {
    case 'edit':
      if (ammo_populate_form('motion', $path_id, $form_state)) {
        // Check access right.
        if (
          isset($form_state['storage']['meeting']) &&
          isset($form_state['storage']['branch_id']) &&
          ammo_edit_access($form_state['storage']['meeting'], $form_state['storage']['branch_id'], 'motion', $form_state['storage']['motion'])
        ) {
          $form_state['storage']['first_step'] = 4;
        }
        else {
          return;
        }
      }
      else {
        return;
      }
      break;
    case 'advice':
      if (ammo_admin_access()) {
        if (ammo_populate_form('motion', $path_id, $form_state)) {
          if (ammo_support_access($form_state['storage']['meeting']) || ammo_superadmin_access()) {
            $form_state['storage']['first_step'] = 6;
          }
          else {
            return;
          }
        }
        else {
          return;
        }
      }
      else {
        return;
      }
      break;
    case 'view':
      $form = ammo_view_entity_form('motion', $path_id);
      return $form;
      break;
    case 'delete':
      if (ammo_superadmin_access()) {
        if (ammo_populate_form('motion', $path_id, $form_state)) {
          $form = ammo_delete_entity_form('motion', $path_id);
          return $form;
        }
      }
      return;
      break;
    }
  }
  else {
    $meetings = ammo_get_meetings('submit');
    if (empty($meetings) && !ammo_admin_access()) {
      // Instead of showing a warning notice and returning, we'll show the page with info text
      $submissions_closed = TRUE;
    }
  }

  // Always set first step.
  if (empty($form_state['storage']['first_step'])) {
    $form_state['storage']['first_step'] = 1;
  }

  // Set data input step for changes check.
  $form_state['storage']['changes_step'] = 5;

  // Set laste page depending on permissions.
  $form_state['storage']['last_step'] = 5;
  if (ammo_admin_access() && !empty($path_id)) {
    $form_state['storage']['last_step'] = 6;
  }

  // If $form_state['step'] is not set, we set it to the first step.
  $form_state['step'] = isset($form_state['step']) ? $form_state['step'] : $form_state['storage']['first_step'];

  // Add a wrapper div that will be used by the Form API to update the form using AJAX
  $form['#prefix'] = '<div id="ajax_form_multistep_form">';
  $form['#suffix'] = '</div>';

  $form['title'] = array(
    '#type' => 'item',
  );

  // Handle submissions closed case
  if (isset($submissions_closed) && $submissions_closed) {
    $form['title']['#markup'] = '<h2>Motie indienen</h2>';
    $form['closed_info'] = array(
      '#type' => 'item',
      '#markup' => '<p><strong>Let op:</strong> De termijn voor het indienen van moties is verstreken. Het is niet meer mogelijk om nieuwe moties in te dienen. Voor aanpassingen kun je contact opnemen met het secretariaat.</p>',
    );
    return $form;
  }

  // Depending on which step of the form we are on, we output different form fields
  if ($form_state['step'] == 1) {
    $form['title']['#markup'] = '<h2>Motie indienen</h2>';
    $options = ammo_get_meetings('submit');
    if (count($options) == 1) {
      $form_state['step'] ++;
      $form_state['storage']['meeting'] = key($options);
      $form_state['storage']['meeting_display_name'] = current($options);
      
      // Check if we should skip the confirmation step for this meeting
      $meeting = entity_load_single('meeting', $form_state['storage']['meeting']);
      if ($meeting && !empty($meeting->disable_confirmation_checkbox)) {
        $form_state['step']++; // Skip step 2 (confirmation)
      }
    }
    else {
      ammo_select_meeting_form($form, $form_state, $options);
    }
  }

  if ($form_state['step'] == 2) {
    $form['title']['#markup'] = '<h2>Motie indienen</h2>';
    // Check if confirmation should be shown for this meeting
    if (isset($form_state['storage']['meeting'])) {
      $meeting = entity_load_single('meeting', $form_state['storage']['meeting']);
      if ($meeting && !empty($meeting->disable_confirmation_checkbox)) {
        // Skip this step - go directly to next step
        $form_state['step']++;
        $form_state['active_steps'][$form_state['step'] - 1] = $form_state['step'] - 1;
        $form_state['rebuild'] = TRUE;
        return $form;
      }
    }
    ammo_checked_form($form, $form_state, 'motion');
  }

  if ($form_state['step'] == 3) {
    $form['title']['#markup'] = '<h2>Motie indienen</h2>';
    $meeting = entity_load_single('meeting', $form_state['storage']['meeting']);
    $options = ammo_afdelingen_access($meeting->admin_owner);
    if (empty($options)) {
      $options = array();
      // Show message that no branches are available
      drupal_set_message('Geen afdelingen beschikbaar voor dit evenement. Neem contact op met de beheerder.', 'warning');
      return $form;
    }
    if (count($options) == 1) {
      $form_state['step'] += 2; // Skip both branch selection and owner selection steps  
      $form_state['storage']['branch_id'] = key($options);
      $form_state['storage']['branch_display_name'] = current($options);
      
      // Don't create entity yet - wait until user provides content in step 5
      $form_state['storage']['single_branch_auto_selected'] = TRUE;
    }
    else {
      ammo_select_afdeling_form($form, $form_state, 'motion', $options);
      $form['branch_id']['#prefix'] =
        '<p>Bijeenkomst: ' . check_plain($form_state['storage']['meeting_display_name']) . '</p>';
    }
  }

  if ($form_state['step'] == 4) {
    $form['title']['#markup'] = '<h2>Motie bewerken</h2>';
    ammo_select_owner_form('motion', $form, $form_state);
  }

  if ($form_state['step'] == 5) {
    $form['title']['#markup'] = '<h2>Motie bewerken</h2>';
    
    // Show selected meeting and branch info
    $branch_info = '<p><strong>Bijeenkomst:</strong> ' . check_plain($form_state['storage']['meeting_display_name']) . '<br/>';
    $branch_info .= '<strong>Afdeling:</strong> ' . check_plain($form_state['storage']['branch_display_name']) . '</p>';
    
    $form['info_display'] = array(
      '#type' => 'item',
      '#markup' => $branch_info,
    );
    
    $ammo_default = ammo_default($form_state, 'consideration_opening');
    $default = (empty($ammo_default)) ? 'Overwegende dat' : $ammo_default;
    $form['consideration_opening'] = array(
      '#type' => 'textfield',
      '#title' => 'Aanleiding / context',
      '#description' => 'Vul hier de openingstekst van de overweging in (Bijv. &lsquo;Omdat&rsquo;',
      '#default_value' => $default,
      '#size' => 50,
      '#maxlength' => 80,
    );

    $form['consideration_text'] = array(
      '#type' => 'textarea',
      '#title' => '',
      '#description' => 'Vul hier de aanleiding van de motie in.',
      '#default_value' => ammo_default($form_state, 'consideration_text'),
    );

    $ammo_default = ammo_default($form_state, 'follow_up_opening');
    $default = (empty($ammo_default)) ? 'Verzoekt het congres' : $ammo_default;
    $form['follow_up_opening'] = array(
      '#type' => 'textfield',
      '#title' => 'Oproep',
      '#description' => 'Vul hier de openingstekst van het vervolg in (Bijv. &lsquo;Verzoekt het congres&rsquo;)',
      '#default_value' => $default,
      '#size' => 50,
      '#maxlength' => 80,
    );

    $form['follow_up_text'] = array(
      '#type' => 'textarea',
      '#title' => '',
      '#description' => 'Vul hier in wat er moet er gebeuren',
      '#default_value' => ammo_default($form_state, 'follow_up_text'),
    );

    $form['supplement'] = array(
      '#type' => 'textarea',
      '#title' => 'Toelichting',
      '#default_value' => ammo_default($form_state, 'supplement'),
    );
  }

  if ($form_state['step'] == 6) {
    $form['title']['#markup'] = '<h2>Motie advies</h2>';
    $form['item'] = array(
      '#type' => 'item',
      '#markup' => theme('motion', array('entity_id' => $form_state['storage']['motion'], 'no_links' => TRUE)),
    );

    $form['state'] = array(
      '#type' => 'select',
      '#title' => 'Status',
      '#options' => ammo_states(),
      '#empty_option' => 'kies:',
      '#default_value' => ammo_default($form_state, 'state'),
    );
    $form['state_supplement'] = array(
      '#type' => 'textarea',
      '#title' => 'Toelichting statuswijziging',
      '#default_value' => ammo_default($form_state, 'state_supplement'),
    );

    $form['advice'] = array(
      '#type' => 'select',
      '#title' => 'Advies',
      '#name' => 'advice',
      '#options' => ammo_motion_advice(),
      '#empty_option' => 'kies:',
      '#default_value' => ammo_default($form_state, 'advice'),
    );

    $form['texts'] = array(
      '#type' => 'fieldset',
      '#states' => array(
        'visible' => array(
          ':input[name="advice"]' => array('value' => 'accept_amended'),
        ),
      ),
    );

    if (!empty($form_state['storage']['advice_consideration_opening'])) {
      $form['texts']['original_consideration_opening'] = array(
        '#type' => 'item',
        '#markup' => '<p><strong>Overweging openingszin origineel</strong><br/>' . check_plain($form_state['storage']['advice_consideration_opening']),
      );
    }
    if (!empty($form_state['storage']['advice_consideration_text'])) {
      $form['texts']['original_consideration_text'] = array(
        '#type' => 'item',
        '#markup' => '<p><strong>Overweging inhoud orgineel</strong><br/>' . check_plain($form_state['storage']['advice_consideration_text']),
      );
    }
    if (!empty($form_state['storage']['advice_follow_up_opening'])) {
      $form['texts']['original_follow_up_opening'] = array(
        '#type' => 'item',
        '#markup' => '<p><strong>Vervolg openingszin orgineel</strong><br/>' . check_plain($form_state['storage']['advice_follow_up_opening']),
      );
    }
    if (!empty($form_state['storage']['advice_follow_up_text'])) {
      $form['texts']['original_follow_up_text'] = array(
        '#type' => 'item',
        '#markup' => '<p><strong>Vervolg inhoud orgineel</strong><br/>' . check_plain($form_state['storage']['advice_follow_up_text']),
      );
    }

    $form['texts']['advice_consideration_opening'] = array(
      '#type' => 'textfield',
      '#title' => 'Overweging openingszin in advies',
      '#default_value' => ammo_default($form_state, 'advice_consideration_opening'),
      '#size' => 50,
      '#maxlength' => 80,
    );
    $form['texts']['advice_consideration_text'] = array(
      '#type' => 'textarea',
      '#title' => 'Overweging inhoud in advies',
      '#default_value' => ammo_default($form_state, 'advice_consideration_text'),
    );
    $form['texts']['advice_follow_up_opening'] = array(
      '#type' => 'textfield',
      '#title' => 'Vervolg openingszin in advies',
      '#default_value' => ammo_default($form_state, 'advice_follow_up_opening'),
      '#size' => 50,
      '#maxlength' => 80,
    );
    $form['texts']['advice_follow_up_text'] = array(
      '#type' => 'textarea',
      '#title' => 'Vervolg inhoud in advies',
      '#default_value' => ammo_default($form_state, 'advice_follow_up_text'),
    );

    $form['advice_supplement'] = array(
      '#type' => 'textarea',
      '#title' => 'Toelichting',
      '#default_value' => ammo_default($form_state, 'advice_supplement'),
    );
  }

  // Create a container for our buttons
  $form['buttons'] = array
    (
      '#type' => 'container',
    );
  // If we are on first step, we do not want a back button
  if(
    $form_state['step'] > $form_state['storage']['first_step'] &&
    $form_state['step'] <> 4 &&
    !empty($form_state['active_steps'])
  )
  {
    $form['buttons']['back'] = array
      (
        '#type' => 'submit',
        '#value' => 'Terug',
        '#name' => 'back',
        '#limit_validation_errors' => array(),
        '#submit' => array('ammo_motion_form_back_submit'),
        '#ajax' => array
        (
          'wrapper' => 'ajax_form_multistep_form',
          'callback' => 'ammo_motion_form_ajax_callback',
        ),
        '#id' => 'back',
      );
  }
  // Create submit button.
  if ($form_state['step'] !== $form_state['storage']['last_step'])
  {
    $button_text = 'Volgende';
  }
  else {
    $button_text = 'Indienen';
  }
  $form['buttons']['forward'] = array
    (
      '#type' => 'submit',
      '#value' => $button_text,
      '#name' => 'action_button',
      '#validate' => array('ammo_motion_form_validate'),
      '#submit' => array('ammo_motion_form_submit'),
      '#id' => 'action_button',
      '#ajax' => array
      (
        // We pass in the wrapper we created at the start of the form
        'wrapper' => 'ajax_form_multistep_form',
        // We pass a callback function we will use later to render the form for the user
        'callback' => 'ammo_motion_form_ajax_callback',
      ),
    );
  return $form;
}

function ammo_motion_form_validate($form, &$form_state)
{
  $values = $form_state['values'];
  $step = $form_state['step'];

  // First we set up a switch for each of our three steps
  switch($step) {
  case 1:
    // Meeting selection - no validation needed as dropdown is required
    break;
  case 2:
    // Only validate the confirmation checkbox if it exists in the form
    if (isset($form['checked']) && empty($values['checked'])) {
      form_set_error('checked', 'U moet bevestigen dat u hebt gecontroleerd dat er niet al een vergelijkbare motie is ingediend.');
    }
    break;
  case 5:
    if (empty($values['consideration_opening'])) {
      form_set_error('consideration_opening', 'Vul de overweging openingszin in.');
    }
    if (empty($values['consideration_text'])) {
      form_set_error('consideration_text', 'Vul de overweging inhoud in.');
    }
    if (empty($values['follow_up_opening'])) {
      form_set_error('follow_up_opening', 'Vul de vervolg openingszin in.');
    }
    if (empty($values['follow_up_text'])) {
      form_set_error('follow_up_text', 'Vul de vervolg inhoud in.');
    }
    break;
  }
}

function ammo_motion_form_back_submit($form, &$form_state)
{
  // Go step back.
  $form_state['step'] = array_pop($form_state['active_steps']);

  // Because this is a multi-step form, we always need to rebuild the form
  // on every step, even after submission. This allows our form to be completely
  // Ajaxified without ever requiring a page load.
  $form_state['rebuild'] = TRUE;
}

function ammo_motion_form_submit($form, &$form_state)
{
  // If validation failed, just rebuild the form and exit.
  if (form_get_errors()) {
    $form_state['rebuild'] = TRUE;
    return;
  }

  $values = $form_state['values'];
  $storage = $form_state['storage'];
  $trigger = $form_state['triggering_element']['#name'];
  $step = $form_state['step'];

  switch ($trigger) {
  case 'action_button':
    switch ($step) {
    case 1:
      $form_state['storage']['meeting'] = $values['meeting'];
      $form_state['storage']['meeting_display_name'] = $form['meeting']['#options'][$values['meeting']];
      
      // Check if we should skip the confirmation step for this meeting
      $meeting = entity_load_single('meeting', $form_state['storage']['meeting']);
      if ($meeting && !empty($meeting->disable_confirmation_checkbox)) {
        $form_state['step']++; // This will make it skip step 2 in the normal increment logic
      }
      break;
    case 2:
      // Confirmation step - no special processing needed, just continue
      break;
    case 3:
      $form_state['storage']['branch_id'] = $values['branch_id'];
      $form_state['storage']['branch_display_name'] = $form['branch_id']['#options'][$values['branch_id']];
      
      // For admins creating new proposals, skip the owner confirmation step
      if (ammo_admin_access() && empty($path_id)) {
        $form_state['step']++; // Skip step 4 (owner confirmation) and go directly to step 5 (content)
        $form_state['storage']['admin_branch_selected'] = TRUE;
      }
      break;
    case 4:
      // Remove entity creation from here - only handle ownership confirmation
      if ($values['backed']) {
        // Store ownership decision, entity will be created later when we have content
        $form_state['storage']['ownership_confirmed'] = TRUE;
      }
      $form_state['storage']['new'] = FALSE;
      break;
    case 5:
      // Create motion entity now that we have valid content to save
      if (empty($storage['motion'])) {
        $form_state['storage']['state'] = 'concept';
        $mapping = array(
          'meeting_id' => 'meeting',
          'state' => 'state',
        );
        $new_motion = ammo_store('motion', NULL, $mapping, $form, $form_state);
        $form_state['storage']['motion'] = $new_motion->id;
        $form_state['storage']['new'] = TRUE;
        
        // Update $storage reference after creating entity
        $storage = $form_state['storage'];
        
        // Set ownership based on how branch was selected
        if (!empty($form_state['storage']['single_branch_auto_selected']) || !empty($form_state['storage']['admin_branch_selected'])) {
          $entity = ammo_save_relation('owner', 'motion', $form_state['storage']['motion'], $form_state['storage']['branch_id'], 'branch', $form_state['storage']['branch_display_name'], $form_state['storage']['branch_id'], $form_state['storage']['branch_display_name']);
        } elseif (!empty($form_state['storage']['ownership_confirmed'])) {
          $entity = ammo_save_relation('owner', 'motion', $form_state['storage']['motion'], $form_state['storage']['branch_id'], 'branch', $form_state['storage']['branch_display_name'], $form_state['storage']['branch_id'], $form_state['storage']['branch_display_name']);
        }
      }
      
      // Save as submitted
      $form_state['storage']['state'] = 'submitted';
      $form_state['values']['state'] = 'submitted';
      
      $mapping = array(
        'consideration_opening' => 'consideration_opening',
        'consideration_body' => 'consideration_text',
        'follow_up_opening' => 'follow_up_opening',
        'follow_up_body' => 'follow_up_text',
        'supplement' => 'supplement',
        'state' => 'state',
      );
      $motion = ammo_store('motion', $storage['motion'], $mapping, $form, $form_state);
      // Reset storage variable.
      $values = $form_state['values'];
      $storage = $form_state['storage'];
      $motion = entity_load_single('motion', $storage['motion']);
      $motion_number = $motion->motion_id ?: $motion->id;
      drupal_set_message('<p>Motie ' . $motion_number . ' is succesvol opgeslagen. E-mail is verzonden.</p>');
      break;
    case 6:
      $mapping = array(
        'advice' => 'advice',
        'advice_supplement' => 'advice_supplement',
        'advice_consideration_opening' => 'advice_consideration_opening',
        'advice_consideration_body' => 'advice_consideration_text',
        'advice_follow_up_opening' => 'advice_follow_up_opening',
        'advice_follow_up_body' => 'advice_follow_up_text',
        'state' => 'state',
        'state_supplement' => 'state_supplement',
      );
      ammo_store('motion', $storage['motion'], $mapping, $form, $form_state);
      drupal_set_message('Het advies is opgeslagen.');
      break;
    }
    // Set next page, or finish form.
    if ($storage['last_step'] == $step) {
      // Redirect user if destination is set.
      ammo_ajax_redirect($form_state['storage']['destination']['destination']);
      // Otherwise reset form.
      $form_state['step'] = 1;
      $form_state['storage'] = array();
      $form_state['storage']['first_step'] = 1;
    }
    else {
      // Increase the step by one, to move on to the next step
      $form_state['active_steps'][$form_state['step']] = $form_state['step'];
      $form_state['step']++;
    }
    break;
  case 'delete_amot':
    $motion = entity_load_single('motion', $storage['motion']);
    $motion->state = 'blocked';
    $result = entity_save('motion', $motion);
    if ($result) {
      drupal_set_message('Motie is verwijderd.');
      ammo_delete_mail('motion', $motion->id, $motion->meeting_id, $values['mail_text']);
    }
    else {
      drupal_set_message('Het was niet mogelijk om de motie te verwijderen.', 'error');
    }
    drupal_goto();
  }

  $form_state['rebuild'] = TRUE;
}

function ammo_motion_form_ajax_callback($form, &$form_state)
{
  return $form;
}
