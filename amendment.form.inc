<?php

function ammo_amendment_form($form, &$form_state, $form_action = NULL, $path_id = NULL)
{
  global $user;

  // Store destination before ajax call.
  ammo_set_destination($form_state);

  // Check if amendment is set in path and preset form if so.
  if (!empty($path_id) && !empty($form_action) && empty($form_state['storage']['first_step'])) {
    switch ($form_action) {
    case 'edit':
      if (ammo_populate_form('amendment', $path_id, $form_state)) {
        $meeting_id = ammo_get_meeting_id('amendment', $form_state['storage']['amendment']);
        // Check access right.
        if (
          $meeting_id &&
          isset($form_state['storage']['branch_id']) &&
          ammo_edit_access($meeting_id, $form_state['storage']['branch_id'], 'amendment', $form_state['storage']['amendment'])
        ) {
          $form_state['storage']['first_step'] = 5;
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
        if (ammo_populate_form('amendment', $path_id, $form_state)) {
          $meeting_id = ammo_get_meeting_id('amendment', $form_state['storage']['amendment']);
          if (ammo_support_access($meeting_id) || ammo_superadmin_access()) {
            $form_state['storage']['first_step'] = 7;
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
      $form = ammo_view_entity_form('amendment', $path_id);
      return $form;
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

  // Set laste page depending on permissions.
  $form_state['storage']['last_step'] = 6; // Confirmation step
  if (ammo_admin_access() && !empty($path_id)) {
    $form_state['storage']['last_step'] = 7; // Admin advice step
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
    $form['title']['#markup'] = '<h2>Wijzigingsvoorstel indienen</h2>';
    $form['closed_info'] = array(
      '#type' => 'item',
      '#markup' => '<p><strong>Let op:</strong> De termijn voor het indienen van wijzigingsvoorstellen is verstreken. Het is niet meer mogelijk om nieuwe wijzigingsvoorstellen in te dienen. Voor aanpassingen kun je contact opnemen met het secretariaat.</p>',
    );
    return $form;
  }

  // Depending on which step of the form we are on, we output different form fields
  if ($form_state['step'] == 1) {
    $form['title']['#markup'] = '<h2>Wijzigingsvoorstel indienen</h2>';
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
    $form['title']['#markup'] = '<h2>Wijzigingsvoorstel indienen</h2>';
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
    ammo_checked_form($form, $form_state, 'amendment');
  }

  if ($form_state['step'] == 3) {
    $form['title']['#markup'] = '<h2>Wijzigingsvoorstel indienen</h2>';
    $options = ammo_get_documents($form_state);
    if (count($options) == 1) {
      $form_state['step'] ++;
      $form_state['storage']['document'] = key($options);
      $form_state['storage']['document_display_name'] = current($options);
    }
    else {
      $form['document'] = array(
        '#type' => 'select',
        '#title' => 'Document',
        '#description' => 'Kies het document wat u wilt amenderen.',
        '#options' => $options,
        '#default_value' => ammo_default($form_state, 'document'),
        '#required' => true,
        '#attributes' => array('onchange' => "jQuery('#action_button').mousedown()"),
        '#prefix' => '<p>Bijeenkomst: ' . check_plain($form_state['storage']['meeting_display_name']) . '</p>',
      );
    }
  }

  if ($form_state['step'] == 4) {
    $form['title']['#markup'] = '<h2>Wijzigingsvoorstel indienen</h2>';
    $meeting = entity_load_single('meeting', $form_state['storage']['meeting']);
    $options = ammo_afdelingen_access($meeting->admin_owner);
    if (empty($options)) {
      $options = array();
      // Show message that no branches are available
      drupal_set_message('Geen afdelingen beschikbaar voor dit evenement. Neem contact op met de beheerder.', 'warning');
      return $form;
    }
    if (count($options) == 1) {
      $form_state['step']++; // Skip branch selection step and go to content (step 5)
      $form_state['storage']['branch_id'] = key($options);
      $form_state['storage']['branch_display_name'] = current($options);
      
      // Don't create entity yet - wait until user provides content in step 5
      $form_state['storage']['single_branch_auto_selected'] = TRUE;
    }
    else {
      ammo_select_afdeling_form($form, $form_state, 'amendment', $options);
      $form['branch_id']['#prefix'] =
        '<p>Bijeenkomst: ' . check_plain($form_state['storage']['meeting_display_name']) . '<br/>' .
        'Document: ' . check_plain($form_state['storage']['document_display_name'])  . '</p>';
    }
  }

  if ($form_state['step'] == 5) {
    $form['title']['#markup'] = '<h2>Wijzigingsvoorstel bewerken</h2>';
    
    // Show selected meeting, document and branch info
    $branch_info = '<p><strong>Bijeenkomst:</strong> ' . check_plain($form_state['storage']['meeting_display_name']) . '<br/>';
    $branch_info .= '<strong>Document:</strong> ' . check_plain($form_state['storage']['document_display_name']) . '<br/>';
    $branch_info .= '<strong>Afdeling:</strong> ' . check_plain($form_state['storage']['branch_display_name']) . '</p>';
    
    $form['info_display'] = array(
      '#type' => 'item',
      '#markup' => $branch_info,
    );
    
    // Get document constraints for each field
    $chapter_description = '';
    $page_description = '';
    $line_description = 'Het regelnummer waar de tekst begint die gewijzigd moet worden, of het regelnummer vanaf waar tekst moet worden toegevoegd.';
    
    if (isset($form_state['storage']['document'])) {
      $document = entity_load_single('document', $form_state['storage']['document']);
      if ($document) {
        // Chapter field description with constraints
        if (!empty($document->chapter_min) || !empty($document->chapter_max)) {
          if (!empty($document->chapter_min) && !empty($document->chapter_max)) {
            $chapter_description = 'Toegestaan: ' . $document->chapter_min . '-' . $document->chapter_max . '.';
          } elseif (!empty($document->chapter_min)) {
            $chapter_description = 'Minimaal: ' . $document->chapter_min . '.';
          } elseif (!empty($document->chapter_max)) {
            $chapter_description = 'Maximaal: ' . $document->chapter_max . '.';
          }
        }
        
        // Page field description with constraints
        if (!empty($document->page_min) || !empty($document->page_max)) {
          if (!empty($document->page_min) && !empty($document->page_max)) {
            $page_description = 'Toegestaan: ' . $document->page_min . '-' . $document->page_max . '.';
          } elseif (!empty($document->page_min)) {
            $page_description = 'Minimaal: ' . $document->page_min . '.';
          } elseif (!empty($document->page_max)) {
            $page_description = 'Maximaal: ' . $document->page_max . '.';
          }
        }
        
        // Line field description with constraints
        if (!empty($document->line_max)) {
          $line_description .= ' Maximaal toegestaan regelnummer: ' . $document->line_max . '.';
        }
      }
    }
    
    $form['chapter'] = array(
      '#type' => 'textfield',
      '#title' => 'Hoofdstuk nr.',
      '#default_value' => ammo_default($form_state, 'chapter'),
      '#description' => $chapter_description,
    );
    $form['page'] = array(
      '#type' => 'textfield',
      '#title' => 'Pagina nummer',
      '#default_value' => ammo_default($form_state, 'page'),
      '#description' => $page_description,
    );
    $form['line'] = array(
      '#type' => 'textfield',
      '#title' => 'Regel nummer (begin)',
      '#description' => $line_description,
      '#default_value' => ammo_default($form_state, 'line'),
    );
    $form['amendment_text'] = array(
      '#type' => 'textarea',
      '#title' => 'Tekst wijzigingsvoorstel',
      '#default_value' => ammo_default($form_state, 'amendment_text'),
    );
    $form['supplement'] = array(
      '#type' => 'textarea',
      '#title' => 'Toelichting',
      '#default_value' => ammo_default($form_state, 'supplement'),
    );
    // Only show submission type choice to admins
    if (ammo_admin_access()) {
      $form['submission_type'] = array(
        '#type' => 'radios',
        '#title' => 'Opslaan als',
        '#options' => array(
          'draft' => 'Concept',   
          'submit' => 'Definitief',
        ),
        '#default_value' => ammo_default($form_state, 'submission_type') ? ammo_default($form_state, 'submission_type') : 'submit',
        '#description' => 'Een concept-voorstel wordt nog niet getoond en je kunt het later nog bewerken. Kies voor definitief als je het voorstel wil publiceren. Je kunt dat niet ongedaan maken.',
      );
    } else {
      // For non-admins, set submission type as hidden field with fixed value
      $form['submission_type'] = array(
        '#type' => 'hidden',
        '#value' => 'submit',
      );
    }
  }

  if ($form_state['step'] == 6) {
    $form['title']['#markup'] = '<h2>Bevestiging</h2>';
    
    // Show summary of what will be submitted (only for definitive submissions)
    $submission_type = ammo_default($form_state, 'submission_type') ?: 'submit';
    $submission_text = 'definitief'; // This step only shows for definitive submissions
    
    $summary = '<div class="amendment-summary">';
    $summary .= '<p><strong>Bijeenkomst:</strong> ' . check_plain($form_state['storage']['meeting_display_name']) . '</p>';
    $summary .= '<p><strong>Document:</strong> ' . check_plain($form_state['storage']['document_display_name']) . '</p>';
    $summary .= '<p><strong>Hoofdstuk:</strong> ' . check_plain(ammo_default($form_state, 'chapter')) . '</p>';
    $summary .= '<p><strong>Pagina:</strong> ' . check_plain(ammo_default($form_state, 'page')) . '</p>';
    $summary .= '<p><strong>Regel:</strong> ' . check_plain(ammo_default($form_state, 'line')) . '</p>';
    $summary .= '<p><strong>Wijzigingsvoorstel:</strong><br/>' . nl2br(check_plain(ammo_default($form_state, 'amendment_text'))) . '</p>';
    if (ammo_default($form_state, 'supplement')) {
      $summary .= '<p><strong>Toelichting:</strong><br/>' . nl2br(check_plain(ammo_default($form_state, 'supplement'))) . '</p>';
    }
    $summary .= '<p><strong>Status:</strong> ' . $submission_text . '</p>';
    $summary .= '</div>';
    
    $form['summary'] = array(
      '#type' => 'item',
      '#markup' => $summary,
    );
    
    $confirm_text = 'Weet u zeker dat u dit wijzigingsvoorstel definitief wilt indienen? Dit kan niet ongedaan worden gemaakt.';
    
    $form['confirm'] = array(
      '#type' => 'checkbox',
      '#title' => $confirm_text,
      '#required' => TRUE,
    );
  }

  if ($form_state['step'] == 7) {
    $form['title']['#markup'] = '<h2>Wijzigingsvoorstel advies</h2>';
    $form['item'] = array(
      '#type' => 'item',
      '#markup' => theme('amendment', array('entity_id' => $form_state['storage']['amendment'], 'no_links' => TRUE)),
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
      '#options' => ammo_amendment_advice(),
      '#empty_option' => 'kies:',
      '#default_value' => ammo_default($form_state, 'advice'),
    );
    $form['advice_supplement'] = array(
      '#type' => 'textarea',
      '#title' => 'Toelichting advies',
      '#default_value' => ammo_default($form_state, 'advice_supplement'),
    );
  }

  // Create a container for our buttons
  $form['buttons'] = array
    (
      '#type' => 'container',
    );
  // If we are on first step, we do not want a back button
  if (
    $form_state['step'] > $form_state['storage']['first_step'] &&
    !empty($form_state['active_steps'])
  )
  {
    $form['buttons']['back'] = array
      (
        '#type' => 'submit',
        '#value' => 'Terug',
        '#name' => 'back',
        '#limit_validation_errors' => array(),
        '#submit' => array('ammo_amendment_form_back_submit'),
        '#ajax' => array
        (
          'wrapper' => 'ajax_form_multistep_form',
          'callback' => 'ammo_amendment_form_ajax_callback',
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
      '#validate' => array('ammo_amendment_form_validate'),
      '#submit' => array('ammo_amendment_form_submit'),
      '#id' => 'action_button',
      '#ajax' => array
      (
        // We pass in the wrapper we created at the start of the form
        'wrapper' => 'ajax_form_multistep_form',
        // We pass a callback function we will use later to render the form for the user
        'callback' => 'ammo_amendment_form_ajax_callback',
      ),
    );
  return $form;
}

function ammo_amendment_form_validate($form, &$form_state) {
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
      form_set_error('checked', 'U moet bevestigen dat u hebt gecontroleerd dat er niet al een vergelijkbaar wijzigingsvoorstel is ingediend.');
    }
    break;
  case 5:
    // Basic format validation
    if (!preg_match('/^[0-9][0-9]*$/', $values['chapter'])) {
      form_set_error('chapter', 'Hoofdstuk nr. moet een getal zijn.');
    }
    if (!preg_match('/^[0-9][0-9]*$/', $values['page'])) {
      form_set_error('page', 'Pagina nr. moet een getal zijn.');
    }
    if (!preg_match('/^[0-9][0-9]*$/', $values['line'])) {
      form_set_error('line', 'Regelnummer moet een getal zijn.');
    }
    if (empty($values['amendment_text'])) {
      form_set_error('amendment_text', 'Vul de wijzigingsvoorstel tekst in.');
    }
    
    // Range validation against document settings
    if (isset($form_state['storage']['document']) && !form_get_errors()) {
      $chapter = (int)$values['chapter'];
      $page = (int)$values['page'];
      $line = (int)$values['line'];
      
      $validation_errors = ammo_validate_document_ranges($form_state['storage']['document'], $chapter, $page, $line);
      foreach ($validation_errors as $field => $message) {
        form_set_error($field, $message);
      }
    }
    break;
  case 6:
    if (empty($values['confirm'])) {
      form_set_error('confirm', 'U moet bevestigen dat u het wijzigingsvoorstel wilt opslaan/indienen.');
    }
    break;
  }
}

function ammo_amendment_form_back_submit($form, &$form_state)
{
  // Go step back.
  $form_state['step'] = array_pop($form_state['active_steps']);

  // Because this is a multi-step form, we always need to rebuild the form
  // on every step, even after submission. This allows our form to be completely
  // Ajaxified without ever requiring a page load.
  $form_state['rebuild'] = TRUE;
}

function ammo_amendment_form_submit($form, &$form_state)
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
      $form_state['storage']['document'] = $values['document'];
      $form_state['storage']['document_display_name'] = $form['document']['#options'][$values['document']];
      break;
    case 4:
      $form_state['storage']['branch_id'] = $values['branch_id'];
      $form_state['storage']['branch_display_name'] = $form['branch_id']['#options'][$values['branch_id']];
      
      // Branch selected, no additional step needed
      $form_state['storage']['branch_selected'] = TRUE;
      break;
    case 5:
      // Store form data first
      $form_state['storage']['chapter'] = $values['chapter'];
      $form_state['storage']['page'] = $values['page'];
      $form_state['storage']['line'] = $values['line'];
      $form_state['storage']['amendment_text'] = $values['amendment_text'];
      $form_state['storage']['supplement'] = $values['supplement'];
      $form_state['storage']['submission_type'] = $values['submission_type'];
      
      // If saving as draft, create entity and save
      if ($values['submission_type'] === 'draft') {
        // Create amendment entity if it doesn't exist yet
        if (empty($storage['amendment'])) {
          $form_state['storage']['state'] = 'concept';
          $mapping = array(
            'document_id' => 'document',
            'state' => 'state',
          );
          $new_amendment = ammo_store('amendment', NULL, $mapping, $form, $form_state);
          $form_state['storage']['amendment'] = $new_amendment->id;
          $form_state['storage']['new'] = TRUE;
          
          // Update $storage reference after creating entity
          $storage = $form_state['storage'];
          
          // Set ownership based on how branch was selected
          if (!empty($form_state['storage']['single_branch_auto_selected']) || !empty($form_state['storage']['branch_selected'])) {
            $entity = ammo_save_relation('owner', 'amendment', $form_state['storage']['amendment'], $form_state['storage']['branch_id'], 'branch', $form_state['storage']['branch_display_name'], $form_state['storage']['branch_id'], $form_state['storage']['branch_display_name']);
          } elseif (!empty($form_state['storage']['ownership_confirmed'])) {
            $entity = ammo_save_relation('owner', 'amendment', $form_state['storage']['amendment'], $form_state['storage']['branch_id'], 'branch', $form_state['storage']['branch_display_name'], $form_state['storage']['branch_id'], $form_state['storage']['branch_display_name']);
          }
        }
        
        // Save as concept (draft)
        $form_state['storage']['state'] = 'concept';
        $form_state['values']['state'] = 'concept';
        
        $mapping = array(
          'chapter' => 'chapter',
          'page' => 'page',
          'line' => 'line',
          'amendment_text' => 'amendment_text',
          'supplement' => 'supplement',
          'state' => 'state',
        );
        $amendment = ammo_store('amendment', $storage['amendment'], $mapping, $form, $form_state);
        
        // Show success message for draft
        $amendment = entity_load_single('amendment', $storage['amendment']);
        $amendment_number = $amendment->chapter . '.' . ($amendment->chapterized_id ?: $amendment->id);
        drupal_set_message('<p>Het wijzigingsvoorstel ' . $amendment_number . ' is opgeslagen als concept. U kunt het later nog wijzigen.</p>');
        
        // Skip to end - redirect user
        ammo_ajax_redirect($form_state['storage']['destination']['destination']);
        return;
      }
      // For definitive submission, continue to confirmation step (entity created in step 6)
      break;
    case 6:
      // Confirmation step - create entity and save definitive submission
      // Create amendment entity if it doesn't exist yet
      if (empty($storage['amendment'])) {
        $form_state['storage']['state'] = 'concept';
        $mapping = array(
          'document_id' => 'document',
          'state' => 'state',
        );
        $new_amendment = ammo_store('amendment', NULL, $mapping, $form, $form_state);
        $form_state['storage']['amendment'] = $new_amendment->id;
        $form_state['storage']['new'] = TRUE;
        
        // Update $storage reference after creating entity
        $storage = $form_state['storage'];
        
        // Set ownership based on how branch was selected
        if (!empty($form_state['storage']['single_branch_auto_selected']) || !empty($form_state['storage']['branch_selected'])) {
          $entity = ammo_save_relation('owner', 'amendment', $form_state['storage']['amendment'], $form_state['storage']['branch_id'], 'branch', $form_state['storage']['branch_display_name'], $form_state['storage']['branch_id'], $form_state['storage']['branch_display_name']);
        } elseif (!empty($form_state['storage']['ownership_confirmed'])) {
          $entity = ammo_save_relation('owner', 'amendment', $form_state['storage']['amendment'], $form_state['storage']['branch_id'], 'branch', $form_state['storage']['branch_display_name'], $form_state['storage']['branch_id'], $form_state['storage']['branch_display_name']);
        }
      }
      
      // Save as submitted
      $form_state['storage']['state'] = 'submitted';
      $form_state['values']['state'] = 'submitted';
      
      $mapping = array(
        'chapter' => 'chapter',
        'page' => 'page',
        'line' => 'line',
        'amendment_text' => 'amendment_text',
        'supplement' => 'supplement',
        'state' => 'state',
      );
      $amendment = ammo_store('amendment', $storage['amendment'], $mapping, $form, $form_state);
      // Reset storage variable.
      $values = $form_state['values'];
      $storage = $form_state['storage'];
      
      // Show success message for definitive submission
      $amendment = entity_load_single('amendment', $storage['amendment']);
      $amendment_number = $amendment->chapter . '.' . ($amendment->chapterized_id ?: $amendment->id);
      drupal_set_message('<p>Het wijzigingsvoorstel ' . $amendment_number . ' is definitief ingediend voor behandeling. E-mail is verzonden.</p>');
      break;
    case 7:
      $mapping = array(
        'advice' => 'advice',
        'advice_supplement' => 'advice_supplement',
        'state' => 'state',
        'state_supplement' => 'state_supplement',
      );
      ammo_store('amendment', $storage['amendment'], $mapping, $form, $form_state);
      drupal_set_message('Het advies is opgeslagen.');
      break;
    }
    // Set next page, or finish form.
    if ($storage['last_step'] == $step) {
      // Redirect user if destination is set.
      ammo_ajax_redirect($form_state['storage']['destination']['destination']);
      // Reset form.
      $form_state['step'] = 1;
      $form_state['storage'] = array();
      $form_state['storage']['first_step'] = 1;
      // Redirect users to page they came from.
    }
    else {
      // Increase the step by one, to move on to the next step
      $form_state['active_steps'][$form_state['step']] = $form_state['step'];
      $form_state['step']++;
    }
    break;
  }

  $form_state['rebuild'] = TRUE;
}

function ammo_amendment_form_ajax_callback($form, &$form_state)
{
  return $form;
}
