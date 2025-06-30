<?php

/**
 * Form function for amendment ownership management
 */
function ammo_amendment_ownership_form($form, &$form_state, $amendment_id) {
  return ammo_ownership_form($form, $form_state, 'amendment', $amendment_id);
}

/**
 * Form function for motion ownership management
 */
function ammo_motion_ownership_form($form, &$form_state, $motion_id) {
  return ammo_ownership_form($form, $form_state, 'motion', $motion_id);
}

/**
 * Generic ownership management form
 */
function ammo_ownership_form($form, &$form_state, $entity_type, $entity_id) {
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

  // Add CSS class for styling
  $form['#attributes']['class'][] = 'ownership-form';

  // Page title
  $title = ($entity_type == 'amendment') ? 'Wijzigingsvoorstel' : 'Motie';
  $form['title'] = array(
    '#markup' => '<h2>Eigenaarschap beheren - ' . $title . ' #' . $entity_id . '</h2>',
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

  // Current ownership section
  $form['current_ownership'] = array(
    '#type' => 'fieldset',
    '#title' => 'Huidige eigenaarschap',
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  // Get current owners and supporters
  $owners_member = ammo_get_related_contacts($entity_id, $entity_type, 'owner', 'member');
  $owners_branch = ammo_get_related_contacts($entity_id, $entity_type, 'owner', 'branch');
  $first_owner = ammo_get_first_owner($entity_type, $entity_id);

  $current_info = '<div class="current-ownership">';
  if (!empty($first_owner)) {
    $current_info .= '<p><strong>Hoofdindiener:</strong> ' . check_plain($first_owner['contact_display_name']);
    if (!empty($first_owner['branch_display_name'])) {
      $current_info .= ' (' . check_plain($first_owner['branch_display_name']) . ')';
    }
    $current_info .= '</p>';
  }

  if (!empty($owners_branch) && count($owners_branch) > 1) {
    $current_info .= '<p><strong>Medeindieners (afdelingen):</strong></p><ul>';
    $first_displayed = false;
    foreach ($owners_branch as $owner) {
      if (!$first_displayed) {
        $first_displayed = true;
        continue; // Skip first owner as it's already shown above
      }
      $current_info .= '<li>' . check_plain($owner['contact_display_name']) . '</li>';
    }
    $current_info .= '</ul>';
  } elseif (count($owners_branch) <= 1) {
    $current_info .= '<p><em>Geen medeindiende afdelingen</em></p>';
  }

  if (!empty($owners_member)) {
    $current_info .= '<p><strong>Medeindieners (individueel):</strong></p><ul>';
    foreach ($owners_member as $owner) {
      $current_info .= '<li>' . check_plain($owner['contact_display_name']);
      if (!empty($owner['branch_display_name'])) {
        $current_info .= ' (' . check_plain($owner['branch_display_name']) . ')';
      }
      $current_info .= '</li>';
    }
    $current_info .= '</ul>';
  }
  $current_info .= '</div>';

  $form['current_ownership']['current_info'] = array(
    '#markup' => $current_info,
  );

  // Change ownership section
  $form['change_ownership'] = array(
    '#type' => 'fieldset',
    '#title' => 'Eigenaarschap wijzigen',
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  // Get all afdelingen for selection
  $all_afdelingen = ammo_get_afdelingen('all');
  if (empty($all_afdelingen)) {
    $all_afdelingen = array();
  }

  // New main owner
  $form['change_ownership']['new_main_owner'] = array(
    '#type' => 'select',
    '#title' => 'Nieuwe hoofdindiener (afdeling)',
    '#description' => 'Selecteer een nieuwe hoofdindiener. Laat leeg om de huidige te behouden.',
    '#options' => $all_afdelingen,
    '#empty_option' => '- Geen wijziging -',
    '#default_value' => isset($form_state['values']['new_main_owner']) ? $form_state['values']['new_main_owner'] : '',
  );

  // Supporting branches management
  $form['change_ownership']['supporting_branches'] = array(
    '#type' => 'checkboxes',
    '#title' => 'Medeindiende afdelingen',
    '#description' => 'Selecteer welke afdelingen dit voorstel mede indienen.',
    '#options' => $all_afdelingen,
    '#default_value' => isset($form_state['values']['supporting_branches']) ? $form_state['values']['supporting_branches'] : array_keys($owners_branch),
  );

  // Individual members section (display only for admins, but don't allow direct editing)
  if (!empty($owners_member)) {
    $form['change_ownership']['individual_note'] = array(
      '#markup' => '<p><strong>Let op:</strong> Individuele indieners kunnen alleen via het normale systeem worden toegevoegd of verwijderd.</p>',
    );
  }

  // Submit buttons
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Eigenaarschap opslaan',
  );

  $dest = drupal_get_destination();
  $form['actions']['cancel'] = array(
    '#markup' => l('Annuleren', $dest['destination'], array('attributes' => array('class' => array('button')))),
  );

  return $form;
}

/**
 * Validation function for ownership forms
 */
function ammo_amendment_ownership_form_validate($form, &$form_state) {
  ammo_ownership_form_validate($form, $form_state);
}

function ammo_motion_ownership_form_validate($form, &$form_state) {
  ammo_ownership_form_validate($form, $form_state);
}

function ammo_ownership_form_validate($form, &$form_state) {
  $values = $form_state['values'];
  
  // Ensure at least one owner is selected
  $has_main_owner = !empty($values['new_main_owner']);
  $has_supporting_branches = !empty(array_filter($values['supporting_branches']));
  
  // If we're changing the main owner, make sure the new main owner is not also in supporting branches
  if (!empty($values['new_main_owner']) && !empty($values['supporting_branches'][$values['new_main_owner']])) {
    form_set_error('supporting_branches', 'De hoofdindiener kan niet tegelijkertijd medeindiener zijn.');
  }
}

/**
 * Submit function for ownership forms
 */
function ammo_amendment_ownership_form_submit($form, &$form_state) {
  ammo_ownership_form_submit($form, $form_state);
}

function ammo_motion_ownership_form_submit($form, &$form_state) {
  ammo_ownership_form_submit($form, $form_state);
}

function ammo_ownership_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  $entity_type = $form_state['storage']['entity_type'];
  $entity_id = $form_state['storage']['entity_id'];
  
  $changes_made = FALSE;
  
  // Get current ownership
  $current_owners_branch = ammo_get_related_contacts($entity_id, $entity_type, 'owner', 'branch');
  $first_owner = ammo_get_first_owner($entity_type, $entity_id);
  
  // Handle main owner change
  if (!empty($values['new_main_owner'])) {
    // Remove all current branch owners
    foreach ($current_owners_branch as $owner) {
      db_delete('ammo_contact_relations')
        ->condition('target_type', $entity_type)
        ->condition('target_id', $entity_id)
        ->condition('relation_type', 'owner')
        ->condition('contact_type', 'branch')
        ->condition('id', $owner['id'])
        ->execute();
    }
    
    // Add new main owner first
    $new_main_afdeling = ammo_get_branch($values['new_main_owner']);
    if ($new_main_afdeling) {
      ammo_save_relation(
        'owner',
        $entity_type,
        $entity_id,
        $values['new_main_owner'],
        'branch',
        $new_main_afdeling->title->value(),
        $values['new_main_owner'],
        $new_main_afdeling->title->value()
      );
      $changes_made = TRUE;
    }
    
    // Add supporting branches (excluding the main owner)
    $selected_supporting = array_filter($values['supporting_branches']);
    foreach ($selected_supporting as $branch_id) {
      if ($branch_id != $values['new_main_owner']) {
        $afdeling = ammo_get_branch($branch_id);
        if ($afdeling) {
          ammo_save_relation(
            'owner',
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
    }
  } else {
    // No main owner change, just update supporting branches
    $selected_supporting = array_filter($values['supporting_branches']);
    $current_branch_ids = array_keys($current_owners_branch);
    
    // Find branches to add
    $to_add = array_diff($selected_supporting, $current_branch_ids);
    foreach ($to_add as $branch_id) {
      $afdeling = ammo_get_branch($branch_id);
      if ($afdeling) {
        ammo_save_relation(
          'owner',
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
    
    // Find branches to remove (but keep the first owner)
    $to_remove = array_diff($current_branch_ids, $selected_supporting);
    foreach ($to_remove as $branch_id) {
      // Don't remove the first owner unless we're changing the main owner
      if ($first_owner && $branch_id != $first_owner['contact_id']) {
        db_delete('ammo_contact_relations')
          ->condition('target_type', $entity_type)
          ->condition('target_id', $entity_id)
          ->condition('relation_type', 'owner')
          ->condition('contact_type', 'branch')
          ->condition('contact_id', $branch_id)
          ->execute();
        $changes_made = TRUE;
      }
    }
  }
  
  if ($changes_made) {
    $type_name = ($entity_type == 'amendment') ? 'wijzigingsvoorstel' : 'motie';
    drupal_set_message('Het eigenaarschap van ' . $type_name . ' #' . $entity_id . ' is bijgewerkt.');
    
    // Send notification emails about ownership changes
    $meeting_id = ammo_get_meeting_id($entity_type, $entity_id);
    if ($meeting_id) {
      // Note: You might want to create a specific email template for ownership changes
      // For now, we'll use the existing change notification system
      $differences = array(
        'ownership' => array(
          'fieldname' => 'Eigenaarschap',
          'diff' => 'Eigenaarschap aangepast door beheerder',
        ),
      );
      ammo_changes_update_mail($entity_type, $entity_id, $differences, $meeting_id);
    }
  } else {
    drupal_set_message('Geen wijzigingen aangebracht.');
  }
  
  // Redirect back to the proposal
  $dest = drupal_get_destination();
  drupal_goto($dest['destination']);
} 