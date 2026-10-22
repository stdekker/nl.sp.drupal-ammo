<?php

/**
 * The item with its document and meeting, or FALSE when one of them is gone.
 */
function ammo_item_view_load($type, $id) {
  if (!is_scalar($id) || !ctype_digit((string) $id)) {
    return FALSE;
  }
  $item = entity_load_single($type, $id);
  if (!$item) {
    return FALSE;
  }
  $document = NULL;
  $meeting_id = $type === 'motion' ? $item->meeting_id : NULL;
  if ($type === 'amendment' && !empty($item->document_id)) {
    $document = entity_load_single('document', $item->document_id);
    $meeting_id = $document ? $document->meeting_id : NULL;
  }
  // Items can outlive a deleted document or meeting.
  $meeting = empty($meeting_id) ? FALSE : entity_load_single('meeting', $meeting_id);
  if (!$meeting) {
    return FALSE;
  }
  return array('item' => $item, 'document' => $document, 'meeting' => $meeting);
}

/**
 * Whether the user is a member owner or belongs to a branch that owns the item.
 */
function ammo_item_view_is_owner($type, $id) {
  if (ammo_is_related($id, $type, 'owner', 'member')) {
    return TRUE;
  }
  foreach (ammo_get_related_contacts($id, $type, 'owner', 'branch') as $owner) {
    if (ammo_access($owner['contact_id'])) {
      return TRUE;
    }
  }
  return FALSE;
}

/**
 * Visibility of a single item, following the overviews.
 *
 * @param array $loaded
 *   Result of ammo_item_view_load().
 */
function ammo_item_view_access($type, array $loaded) {
  if (!ammo_view_overview_access() && !ammo_add_access()) {
    return FALSE;
  }
  if ($type === 'motion' && !ammo_show_motions()) {
    return FALSE;
  }
  // The overviews show nothing of a hidden meeting, not even to admins.
  if (!empty($loaded['meeting']->hide_meeting)) {
    return FALSE;
  }
  if (ammo_admin_access()) {
    return TRUE;
  }
  $state = $loaded['item']->state;
  if ($state === 'blocked') {
    return FALSE;
  }
  if (!empty($state) && $state !== 'concept') {
    return TRUE;
  }
  return ammo_item_view_is_owner($type, $loaded['item']->id);
}

/**
 * The status suffix the item heading shows for collapsed items, such as " - Ingetrokken".
 */
function ammo_item_view_status_suffix($type, $item) {
  if ($item->state === 'withdrawn') {
    return ' - Ingetrokken';
  }
  if ($type === 'amendment' && $item->state === 'invalid') {
    return ' - Ongeldig';
  }
  if ($type === 'motion' && ammo_assessed_as($item, 'reject')) {
    return ' - Afgewezen';
  }
  return '';
}

function ammo_item_view_page($type, $id) {
  drupal_page_is_cacheable(FALSE);
  $loaded = ammo_item_view_load($type, $id);
  if (!$loaded) {
    return MENU_NOT_FOUND;
  }
  if (!ammo_item_view_access($type, $loaded)) {
    return MENU_ACCESS_DENIED;
  }
  $item = $loaded['item'];
  $meeting = $loaded['meeting'];
  // Shown as the first properties of the item.
  $properties = array('Bijeenkomst' => check_plain($meeting->title));
  if ($type === 'amendment') {
    $properties['Document'] = check_plain($loaded['document']->title);
    $overview = 'ammo/amendments/' . $meeting->id;
    // The amendment overview only loads the selected document and group.
    $group = ammo_amendment_group($item);
    $options = array('query' => array('document' => $item->document_id, 'chapter' => $group === '' ? 0 : $group));
  }
  else {
    $overview = 'ammo/motions/' . $meeting->id;
    $options = array();
  }
  // The page title replaces the heading of the item.
  drupal_set_title(ammo_item_heading($type, $item) . ammo_item_view_status_suffix($type, $item));
  $back = array();
  // Drafts are not part of the overviews.
  if ($item->state !== 'concept') {
    $back[] = l('Alle voorstellen', $overview, $options + array('fragment' => $type . $item->id));
  }
  if (ammo_add_access()) {
    $back[] = l('Mijn afdeling', 'ammo/afdeling/' . $meeting->id);
  }
  return array(
    '#attached' => array('js' => array(drupal_get_path('module', 'ammo') . '/ammo.js')),
    'item' => array('#markup' => '<ul class="ammo-item"><li class="ammo-element ' . drupal_html_class($item->state) . '">' . theme($type, array('entity_id' => $item->id, 'show_heading' => FALSE, 'properties' => $properties)) . '</li></ul>'),
    'dates' => array('#markup' => ammo_item_view_dates($item)),
    'back' => array('#markup' => $back ? '<nav class="ammo-item-back" aria-label="Terug naar"><span class="ammo-item-back-label">Terug naar:</span><ul><li>' . implode('</li><li>', $back) . '</li></ul></nav>' : ''),
  );
}

/**
 * When the item was created and last changed, below the item; parts that are not known are left out.
 */
function ammo_item_view_dates($item) {
  $dates = array();
  if ($created = ammo_item_date($item, 'created')) {
    $dates[] = 'Aangemaakt op ' . $created;
  }
  if ($changed = ammo_item_date($item, 'changed')) {
    $dates[] = 'laatst gewijzigd op ' . $changed;
  }
  return $dates ? '<p class="ammo-item-dates">' . ucfirst(implode(', ', $dates)) . '</p>' : '';
}
