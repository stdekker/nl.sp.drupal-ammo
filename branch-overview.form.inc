<?php

function ammo_branch_overview_form($form, &$form_state, $meeting_id = NULL) {
  drupal_page_is_cacheable(FALSE);
  module_load_include('inc', 'ammo', 'branch-overview');

  // Check if there is choice.
  $options = ammo_get_meetings();
  if (empty($meeting_id) && count($options) == 1) {
    $meeting_id = key($options);
  }
  if (empty($meeting_id)) {
    ammo_select_meeting_form($form, $form_state, $options);
  }
  else {
    $form['meeting'] = array(
      '#type' => 'value',
      '#value' => $meeting_id,
    );
  }

  $branches = ammo_branch_overview_branches($_GET);
  if (ammo_admin_access()) {
    $all_branches = ammo_get_afdelingen('all');
    $all_branches = is_array($all_branches) ? $all_branches : array();
    $selected = isset($_GET['afdeling']) && is_scalar($_GET['afdeling']) ? (string) $_GET['afdeling'] : '';
    $form['afdeling'] = array(
      '#type' => 'select',
      '#title' => 'Afdeling',
      '#description' => 'Bekijk het overzicht van een andere afdeling.',
      '#options' => $all_branches,
      '#empty_option' => '- eigen afdeling -',
      '#default_value' => isset($all_branches[$selected]) ? $selected : '',
    );
  }
  if (empty($meeting_id) || ammo_admin_access()) {
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => 'kies',
      '#name' => 'action_button',
      '#id' => 'action_button',
      '#submit' => array('ammo_branch_overview_form_submit'),
    );
  }
  $form['overview'] = array(
    '#type' => 'item',
  );
  if (!empty($meeting_id)) {
    $form['overview']['#markup'] = theme('overview_branch', array('meeting_id' => $meeting_id, 'selected_branches' => $branches));
  }
  return $form;
}

function ammo_branch_overview_form_submit($form, &$form_state) {
  $options = array();
  if (!empty($form_state['values']['afdeling'])) {
    $options['query'] = array('afdeling' => $form_state['values']['afdeling']);
  }
  $form_state['redirect'] = array('ammo/afdeling/' . $form_state['values']['meeting'], $options);
}
