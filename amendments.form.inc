<?php

function ammo_overview_amendments_form($form, &$form_state, $meeting_id = NULL) {
  drupal_page_is_cacheable(FALSE);
  drupal_set_title('Ingediende wijzigingsvoorstellen');
  // Preserve the overview location for amendment actions.
  ammo_set_destination($form_state);

  $form['#attached']['js'] = array(
    drupal_get_path('module', 'ammo') . '/ammo.js',
  );

  $form['#prefix'] = '<div id="ajax_form">';
  $form['#suffix'] = '</div>';

  // Check if there is choice.
  $options = ammo_get_meetings();
  if (!empty($meeting_id) || count($options) == 1) {
    if (empty($meeting_id) && count($options) == 1) {
      $meeting_id = key($options);
    }
    $form['overview'] = array(
      '#type' => 'item',
      '#markup' => theme('overview_amendments', array('meeting_id' => $meeting_id, 'destination' => $form_state['storage']['destination'])),
    );
  }
  else {
    ammo_select_meeting_form($form, $form_state, $options);
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => 'kies',
      '#name' => 'action_button',
      '#id' => 'action_button',
      '#submit' => array('ammo_overview_amendments_form_submit'),
    );
    $form['overview'] = array(
      '#type' => 'item',
    );
  }
  return $form;
}

function ammo_overview_amendments_form_submit($form, &$form_state) {
  $form_state['redirect'] = 'ammo/amendments/' . $form_state['values']['meeting'];
}
