<?php

/**
 * Admin overview of the proposals of a document or chapter, to change the status of the ticked ones at once.
 */
function ammo_status_overview_form($form, &$form_state) {
  drupal_page_is_cacheable(FALSE);
  module_load_include('inc', 'ammo', 'status-overview');
  $selection = ammo_status_overview_selection($_GET);
  $form['#attributes']['class'][] = 'ammo-status-overview';
  $form_state['ammo_selection'] = $selection;
  if (!$selection['meeting']) {
    $form['empty'] = array('#markup' => '<p>Er zijn geen bijeenkomsten.</p>');
    return $form;
  }

  $form['filters'] = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('container-inline', 'ammo-status-filters')),
  );
  if (count($selection['meetings']) > 1) {
    $form['filters']['meeting'] = array(
      '#type' => 'select',
      '#title' => 'Bijeenkomst',
      '#options' => array_map('check_plain', $selection['meetings']),
      '#default_value' => $selection['meeting'],
    );
  }
  if ($selection['documents']) {
    $form['filters']['document'] = array(
      '#type' => 'select',
      '#title' => 'Document',
      '#options' => array_map('check_plain', $selection['documents']),
      '#default_value' => $selection['document'],
    );
  }
  if ($selection['chapters']) {
    $form['filters']['chapter'] = array(
      '#type' => 'select',
      '#title' => check_plain($selection['grouping']['label']),
      '#options' => array('' => 'Alle') + array_map('check_plain', $selection['chapters']),
      '#default_value' => $selection['chapter'],
    );
  }
  $form['filters']['filter'] = array(
    '#type' => 'submit',
    '#value' => 'Filteren',
    '#limit_validation_errors' => array(array('meeting'), array('document'), array('chapter')),
    '#submit' => array('ammo_status_overview_filter_submit'),
  );

  $header = array(
    'number' => 'Nr',
    'location' => 'Locatie',
    'first_owner' => 'Hoofdindiener',
    'text' => 'Voorstel',
    'state' => 'Status',
    'advice' => 'Advies',
    'advice_supplement' => 'Advies toelichting',
    'created' => 'Aangemaakt',
    'changed' => 'Gewijzigd',
    'actions' => 'Acties',
  );
  if ($selection['document'] === 'motions') {
    unset($header['location']);
  }
  $rows = ammo_status_overview_rows($selection);
  $empty = 'Er zijn geen voorstellen.';

  if (!ammo_status_overview_edit_access($selection['meeting'])) {
    $form['closed'] = array('#markup' => '<p>De bijeenkomst is voorbij. Alleen superbeheerders kunnen de status nog wijzigen.</p>');
    foreach ($rows as &$row) {
      $row = array('data' => array_intersect_key($row, $header), 'class' => $row['#attributes']['class']);
    }
    unset($row);
    $form['items'] = array('#theme' => 'table', '#header' => $header, '#rows' => $rows, '#empty' => $empty, '#sticky' => FALSE);
    $form['pager'] = array('#theme' => 'pager');
    return $form;
  }

  $form['bulk'] = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('container-inline', 'ammo-status-bulk')),
  );
  $form['bulk']['state'] = array(
    '#type' => 'select',
    '#title' => 'Nieuwe status',
    '#options' => ammo_editable_states(),
    '#empty_option' => '- kies een status -',
  );
  $form['bulk']['apply'] = array(
    '#type' => 'submit',
    '#value' => 'Status wijzigen voor selectie',
    '#validate' => array('ammo_status_overview_form_validate'),
    '#submit' => array('ammo_status_overview_form_submit'),
  );
  $form['items'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $rows,
    '#empty' => $empty,
    '#sticky' => FALSE,
  );
  $form['pager'] = array('#theme' => 'pager');
  return $form;
}

/**
 * Reloads the page with the chosen filters; another document or meeting starts at all chapters.
 */
function ammo_status_overview_filter_submit($form, &$form_state) {
  $previous = $form_state['ammo_selection'];
  $values = $form_state['values'] + array('meeting' => $previous['meeting'], 'document' => NULL, 'chapter' => '');
  $query = array('meeting' => $values['meeting']);
  if ($values['meeting'] == $previous['meeting']) {
    $query['document'] = $values['document'];
    if ($values['document'] == $previous['document']) {
      $query['chapter'] = $values['chapter'];
    }
  }
  $form_state['redirect'] = array('admin/ammo/status', array('query' => array_filter($query, 'strlen')));
}

/**
 * Requires a status and at least one ticked proposal.
 */
function ammo_status_overview_form_validate($form, &$form_state) {
  if (empty($form_state['values']['state'])) {
    form_set_error('state', 'Kies de nieuwe status.');
  }
  if (!array_filter($form_state['values']['items'])) {
    form_set_error('items', 'Vink de voorstellen aan waarvan je de status wilt wijzigen.');
  }
}

/**
 * Sets the chosen status on every ticked proposal and reports what changed.
 */
function ammo_status_overview_form_submit($form, &$form_state) {
  module_load_include('inc', 'ammo', 'status-overview');
  $selection = $form_state['ammo_selection'];
  $state = $form_state['values']['state'];
  $results = array('changed' => array(), 'unchanged' => array(), 'no_advice' => array(), 'failed' => array());
  foreach (array_filter($form_state['values']['items']) as $key) {
    list($type, $id) = explode(':', $key, 2);
    $entity = entity_load_single($type, $id);
    // Only items of the meeting on screen can be changed here.
    if (!$entity || ammo_get_meeting_id($type, $id) != $selection['meeting']) {
      continue;
    }
    $label = ammo_status_overview_number($type, $entity);
    $results[ammo_status_overview_set_state($type, $id, $state)][] = check_plain($label);
  }
  $states = ammo_states();
  if ($results['changed']) {
    drupal_set_message(format_plural(count($results['changed']), '1 voorstel heeft nu de status @state: !items.', '@count voorstellen hebben nu de status @state: !items.', array('@state' => $states[$state], '!items' => implode(', ', $results['changed']))));
  }
  if ($results['unchanged']) {
    drupal_set_message('Had deze status al: ' . implode(', ', $results['unchanged']) . '.');
  }
  if ($results['no_advice']) {
    drupal_set_message('Niet gewijzigd, want bij de status Beoordeeld geeft het advies de uitkomst en dit voorstel heeft nog geen advies: ' . implode(', ', $results['no_advice']) . '.', 'error');
  }
  if ($results['failed']) {
    drupal_set_message('Niet opgeslagen: ' . implode(', ', $results['failed']) . '.', 'error');
  }
  $form_state['redirect'] = array('admin/ammo/status', array('query' => ammo_status_overview_query($selection)));
}
