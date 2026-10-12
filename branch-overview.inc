<?php

/**
 * Branches whose overview is shown: the user's own, or one picked by an admin.
 */
function ammo_branch_overview_branches(array $selection) {
  if (ammo_admin_access() && isset($selection['afdeling']) && is_scalar($selection['afdeling'])) {
    $all = ammo_get_afdelingen('all');
    $branch_id = (string) $selection['afdeling'];
    if (is_array($all) && isset($all[$branch_id])) {
      return array($branch_id => $all[$branch_id]);
    }
  }
  $own = ammo_afdelingen_access();
  return is_array($own) ? $own : array();
}

/**
 * Owner relations of every item in a meeting that one of the branches owns.
 *
 * Co-owners of other branches are included, because the first owner of an
 * item can only be determined over all of its owner relations.
 */
function ammo_branch_overview_relations($type, $meeting_id, array $branch_ids) {
  $query = db_select('ammo_contact_relations', 'r');
  $query->join('ammo_' . $type . 's', 't', 't.id = r.target_id');
  $query->addField('t', 'id', 'item_id');
  $query->fields('t', array('state'));
  $query->fields('r', array('contact_id', 'contact_type', 'contact_display_name'));
  if ($type === 'amendment') {
    $query->join('ammo_documents', 'd', 'd.id = t.document_id');
    $query->fields('t', array('chapter', 'chapterized_id', 'page', 'line'));
    $query->addField('d', 'title', 'document_title');
    $query->condition('d.meeting_id', $meeting_id);
    $query->orderBy('d.title')->orderBy('t.chapter')->orderBy('t.chapterized_id');
  }
  else {
    $query->fields('t', array('motion_id', 'consideration_body'));
    $query->condition('t.meeting_id', $meeting_id);
    $query->orderBy('t.motion_id');
  }
  $query->condition('r.target_type', $type)->condition('r.relation_type', 'owner');
  if (!ammo_admin_access()) {
    $query->condition('t.state', 'blocked', '<>');
  }
  $owned = db_select('ammo_contact_relations', 'o');
  $owned->addExpression('1');
  $owned->where('o.target_id = r.target_id');
  $owned->condition('o.target_type', $type)->condition('o.relation_type', 'owner')->condition('o.contact_type', 'branch')->condition('o.contact_id', $branch_ids, 'IN');
  $query->exists($owned);
  // The lowest relation id is the first owner, as in ammo_get_first_owner().
  $query->orderBy('t.id')->orderBy('r.id');
  return $query->execute();
}

/**
 * Items per branch, split in submitted and co-submitted.
 *
 * A branch submitted an item when it is the first owner. Member owners take
 * precedence over branch owners, as in ammo_get_first_owner().
 */
function ammo_branch_overview_items($type, $meeting_id, array $branch_ids) {
  $overview = array_fill_keys($branch_ids, array('submitted' => array(), 'co_submitted' => array()));
  if (!$branch_ids) {
    return $overview;
  }
  $items = array();
  foreach (ammo_branch_overview_relations($type, $meeting_id, $branch_ids) as $relation) {
    $relation = (array) $relation;
    $id = $relation['item_id'];
    if (!isset($items[$id])) {
      $items[$id] = $relation + array('members' => array(), 'branches' => array());
    }
    $owners = $relation['contact_type'] === 'member' ? 'members' : 'branches';
    $items[$id][$owners][$relation['contact_id']] = $relation['contact_display_name'];
  }
  foreach ($items as $id => $item) {
    $first_branch_id = key($item['branches']);
    foreach ($branch_ids as $branch_id) {
      if (!isset($item['branches'][$branch_id])) {
        continue;
      }
      if (empty($item['members']) && $first_branch_id == $branch_id) {
        $item['others'] = array_diff_key($item['branches'], array($branch_id => TRUE));
        $overview[$branch_id]['submitted'][$id] = $item;
      }
      // A draft is only shown to the branch that is writing it.
      elseif ($item['state'] !== 'concept') {
        $item['others'] = array(reset($item['members']) ?: reset($item['branches']));
        $overview[$branch_id]['co_submitted'][$id] = $item;
      }
    }
  }
  return $overview;
}

/**
 * Status label, hidden when the meeting hides states like the item templates.
 */
function ammo_branch_overview_state_label($type, $state, $meeting) {
  $always_shown = array('concept', 'withdrawn', $type === 'motion' ? 'rejected' : 'invalid');
  if (!empty($meeting->hide_state) && !ammo_admin_access() && !in_array($state, $always_shown, TRUE)) {
    return '';
  }
  $states = ammo_states();
  return isset($states[$state]) ? $states[$state] : check_plain($state);
}

/**
 * Item number, linked to the page of the item.
 */
function ammo_branch_overview_number_link($type, array $item) {
  if ($type === 'amendment') {
    $number = ($item['chapter'] ?: 'X') . '.' . ($item['chapterized_id'] ?: $item['item_id']);
  }
  else {
    $number = $item['motion_id'] ?: $item['item_id'];
  }
  return l($number, ammo_item_view_path($type, $item['item_id']));
}

function ammo_branch_overview_row($type, array $item, $meeting) {
  $state = ammo_branch_overview_state_label($type, $item['state'], $meeting);
  $cells = array(ammo_branch_overview_number_link($type, $item));
  if ($type === 'amendment') {
    $cells[] = check_plain($item['document_title']);
    $cells[] = $item['chapter'] ? check_plain($item['chapter']) : '-';
    $cells[] = $item['page'] || $item['line'] ? check_plain($item['page'] . ' / ' . $item['line']) : '';
  }
  else {
    $cells[] = check_plain(truncate_utf8(strip_tags($item['consideration_body']), 80, TRUE, TRUE));
  }
  $cells[] = $state;
  $cells[] = $item['others'] ? check_plain(implode(', ', $item['others'])) : '-';
  // A hidden state must not be readable from the row class either.
  return array('data' => $cells, 'class' => $state === '' ? array() : array(drupal_html_class($item['state'])));
}

/**
 * Template data: per branch a submitted and a co-submitted part with tables.
 */
function ammo_branch_overview_page($meeting_id, array $branches) {
  $page = array('branches' => array(), 'meeting_available' => FALSE);
  $meeting = is_numeric($meeting_id) ? entity_load_single('meeting', $meeting_id) : FALSE;
  if (!$meeting || !empty($meeting->hide_meeting)) {
    return $page;
  }
  $page['meeting_available'] = TRUE;
  $types = array('amendment' => array('Wijzigingsvoorstellen', array('Nr', 'Document', 'Hoofdstuk', 'Pagina / regel', 'Status')));
  if (ammo_show_motions() && empty($meeting->no_motions)) {
    $types['motion'] = array('Moties', array('Nr', 'Motie', 'Status'));
  }
  $parts = array('submitted' => array('Ingediend door', 'Mede-indieners'), 'co_submitted' => array('Mede ingediend door', 'Indiener'));
  foreach ($branches as $branch_id => $name) {
    $page['branches'][$branch_id] = array_fill_keys(array_keys($parts), array('tables' => array()));
  }
  foreach ($types as $type => $table) {
    list($caption, $header) = $table;
    $overview = ammo_branch_overview_items($type, $meeting->id, array_keys($branches));
    foreach ($overview as $branch_id => $branch_items) {
      foreach ($parts as $part => $labels) {
        $rows = array();
        foreach ($branch_items[$part] as $item) {
          $rows[] = ammo_branch_overview_row($type, $item, $meeting);
        }
        $page['branches'][$branch_id][$part]['title'] = $labels[0] . ' ' . check_plain(ammo_cleanup_branch_name($branches[$branch_id]));
        $page['branches'][$branch_id][$part]['tables'][$type] = array(
          'caption' => $caption,
          'header' => array_merge($header, array($labels[1])),
          'rows' => $rows,
          'empty' => 'Geen ' . drupal_strtolower($caption) . '.',
          'sticky' => FALSE,
        );
      }
    }
  }
  return $page;
}
