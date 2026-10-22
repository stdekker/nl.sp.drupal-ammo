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
  $query->fields('t', array('state', 'advice'));
  $query->fields('r', array('contact_id', 'contact_type', 'contact_display_name'));
  if ($type === 'amendment') {
    $query->join('ammo_documents', 'd', 'd.id = t.document_id');
    $query->fields('t', array('document_id', 'chapter', 'group_value', 'chapterized_id', 'page', 'line', 'locator_values'));
    $query->addField('d', 'title', 'document_title');
    $query->condition('d.meeting_id', $meeting_id);
    // Shorter group values first, so numeric groups sort as numbers.
    $query->addExpression('LENGTH(t.group_value)', 'group_length');
    $query->orderBy('d.title')->orderBy('d.id')->orderBy('t.chapter')->orderBy('group_length')->orderBy('t.group_value')->orderBy('t.chapterized_id');
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
function ammo_branch_overview_state_label($type, $state, $meeting, $advice = NULL) {
  $always_shown = array('concept', 'withdrawn', $type === 'motion' ? '' : 'invalid');
  // Motions assessed as rejected are folded in on their page, so they always show.
  $rejected_motion = $type === 'motion' && ammo_assessed_as(array('state' => $state, 'advice' => $advice), 'reject');
  if (!empty($meeting->hide_state) && !ammo_admin_access() && !in_array($state, $always_shown, TRUE) && !$rejected_motion) {
    return '';
  }
  $states = ammo_states();
  return isset($states[$state]) ? $states[$state] : check_plain($state);
}

/**
 * Advice label, or an empty string when there is none, the state does not show it, or the meeting hides it from non-admins.
 */
function ammo_branch_overview_advice_label($type, $advice, $meeting, $state) {
  if (empty($advice) || !ammo_state_shows_advice($state) || (!empty($meeting->hide_advice) && !ammo_admin_access())) {
    return '';
  }
  $advices = $type === 'amendment' ? ammo_amendment_advice() : ammo_motion_advice();
  return isset($advices[$advice]) ? $advices[$advice] : check_plain($advice);
}

/**
 * Item number, linked to the page of the item.
 */
function ammo_branch_overview_number_link($type, array $item) {
  if ($type === 'amendment') {
    $number = ammo_amendment_number(array('id' => $item['item_id']) + $item);
  }
  else {
    $number = $item['motion_id'] ?: $item['item_id'];
  }
  return l($number, ammo_item_view_path($type, $item['item_id']));
}

function ammo_branch_overview_row($type, array $item, $meeting) {
  $state = ammo_branch_overview_state_label($type, $item['state'], $meeting, $item['advice']);
  $cells = array(ammo_branch_overview_number_link($type, $item));
  if ($type === 'amendment') {
    $cells[] = check_plain(ammo_amendment_location_short($item, $item['document_id']));
  }
  else {
    $cells[] = check_plain(truncate_utf8(strip_tags($item['consideration_body']), 80, TRUE, TRUE));
  }
  $cells[] = $state;
  $cells[] = ammo_branch_overview_advice_label($type, $item['advice'], $meeting, $item['state']);
  $cells[] = $item['others'] ? check_plain(implode(', ', $item['others'])) : '-';
  // A hidden state must not be readable from the row class either.
  return array('data' => $cells, 'class' => $state === '' ? array() : array(drupal_html_class($item['state'])));
}

/**
 * The meeting of the overview, or FALSE when it does not exist or is hidden.
 */
function ammo_branch_overview_meeting($meeting_id) {
  $meeting = is_numeric($meeting_id) ? entity_load_single('meeting', $meeting_id) : FALSE;
  return $meeting && empty($meeting->hide_meeting) ? $meeting : FALSE;
}

/**
 * Whether the overview lists motions of the meeting.
 */
function ammo_branch_overview_shows_motions($meeting) {
  return ammo_show_motions() && ammo_meeting_has_motions($meeting);
}

/**
 * Items the branches submitted or co-submitted, as listed on the overview, keyed by id.
 */
function ammo_branch_overview_download_items($type, $meeting_id, array $branch_ids) {
  $items = array();
  foreach (ammo_branch_overview_items($type, $meeting_id, $branch_ids) as $branch_items) {
    $items += $branch_items['submitted'] + $branch_items['co_submitted'];
  }
  return $items;
}

/**
 * Downloads the amendments and motions of the overview as one Word file.
 */
function ammo_branch_overview_download($meeting_id) {
  $meeting = ammo_branch_overview_meeting($meeting_id);
  if (!$meeting) {
    return MENU_NOT_FOUND;
  }
  $branches = ammo_branch_overview_branches($_GET);
  $data = ammo_branch_overview_download_data($meeting, $branches);
  if (!$data['documents'] && !$data['motions']) {
    return MENU_NOT_FOUND;
  }
  module_load_include('inc', 'ammo', 'docx');
  $content = ammo_docx_build(ammo_branch_overview_download_blocks($data), ammo_branch_overview_download_footer(REQUEST_TIME));
  if ($content === FALSE) {
    watchdog('ammo', 'The Mijn afdeling download of meeting @meeting could not be written.', array('@meeting' => $meeting->id), WATCHDOG_ERROR);
    return '<p>Het Word-bestand kon niet worden gemaakt. Probeer het later opnieuw.</p>';
  }
  module_load_include('inc', 'ammo', 'print.form');
  $parts = array('ammo', ammo_sanitize_filename($meeting->title));
  if (count($branches) === 1) {
    $parts[] = ammo_sanitize_filename(ammo_cleanup_branch_name(reset($branches)));
  }
  $parts[] = date('Y-m-d--H-i');
  ammo_print_download(implode('-', $parts) . '.docx', $content);
}

/**
 * Submitter and co-submitters of an overview item; a member owner precedes the branches, as in ammo_get_first_owner().
 */
function ammo_branch_overview_download_owners(array $item) {
  $branches = $item['branches'];
  $submitter = $item['members'] ? reset($item['members']) : array_shift($branches);
  return array(
    'submitter' => (string) $submitter,
    'co_submitters' => implode(', ', $branches),
  );
}

/**
 * Data of the download as plain text: the overview's amendments per document and its motions, with status and advice as the overview shows them.
 *
 * The items come from the overview queries, never from the request.
 */
function ammo_branch_overview_download_data($meeting, array $branches) {
  $data = array(
    'title' => $meeting->title,
    'branches' => implode(', ', array_map('ammo_cleanup_branch_name', $branches)),
    'documents' => array(),
    'motions' => array(),
  );
  $branch_ids = array_keys($branches);
  $items = ammo_branch_overview_download_items('amendment', $meeting->id, $branch_ids);
  if ($items) {
    $rows = db_query('SELECT * FROM {ammo_amendments} WHERE id IN (:ids)', array(':ids' => array_keys($items)))->fetchAllAssoc('id');
    $documents = db_query('SELECT id, title FROM {ammo_documents} WHERE id IN (:ids) ORDER BY title, id', array(':ids' => array_unique(array_column($items, 'document_id'))))->fetchAllKeyed();
    foreach ($documents as $document_id => $title) {
      $document_rows = array_filter($rows, function ($row) use ($document_id) {
        return $row->document_id == $document_id;
      });
      $locators = ammo_document_locators($document_id);
      ammo_sort_amendments($document_rows, $locators);
      $amendments = array();
      foreach ($document_rows as $id => $row) {
        $advice = ammo_branch_overview_advice_label('amendment', $row->advice, $meeting, $row->state);
        $amendments[$id] = ammo_branch_overview_download_owners($items[$id]) + array(
          'number' => ammo_amendment_number($row),
          'location' => ammo_amendment_location($row, $locators),
          'text' => $row->amendment_text,
          'supplement' => $row->supplement,
          'state' => ammo_branch_overview_state_label('amendment', $row->state, $meeting, $row->advice),
          'advice' => $advice,
          'advice_supplement' => $advice === '' ? '' : $row->advice_supplement,
        );
      }
      $data['documents'][$document_id] = array('title' => $title, 'amendments' => $amendments);
    }
  }
  if (ammo_branch_overview_shows_motions($meeting)) {
    $items = ammo_branch_overview_download_items('motion', $meeting->id, $branch_ids);
    if ($items) {
      foreach (db_query('SELECT * FROM {ammo_motions} WHERE id IN (:ids) ORDER BY motion_id, id', array(':ids' => array_keys($items))) as $row) {
        $advice = ammo_branch_overview_advice_label('motion', $row->advice, $meeting, $row->state);
        $data['motions'][$row->id] = ammo_branch_overview_download_owners($items[$row->id]) + array(
          'number' => $row->motion_id ?: $row->id,
          'consideration_opening' => $row->consideration_opening,
          'consideration_body' => $row->consideration_body,
          'follow_up_opening' => $row->follow_up_opening,
          'follow_up_body' => $row->follow_up_body,
          'supplement' => $row->supplement,
          'state' => ammo_branch_overview_state_label('motion', $row->state, $meeting, $row->advice),
          'advice' => $advice,
          'advice_supplement' => $advice === '' ? '' : $row->advice_supplement,
        );
      }
    }
  }
  return $data;
}

/**
 * Footer line of the download with the moment it was made.
 */
function ammo_branch_overview_download_footer($timestamp) {
  return 'Gedownload op ' . format_date($timestamp, 'custom', 'j F Y \o\m H:i');
}

/**
 * Word blocks of the download: the meeting and afdeling, a section per document and one for the motions, each on a new page, with a line between proposals.
 */
function ammo_branch_overview_download_blocks(array $data) {
  $blocks = array(ammo_docx_block($data['title'], 'Heading1'));
  if ($data['branches'] !== '') {
    $blocks[] = ammo_docx_block('Voorstellen van ' . $data['branches'] . '.');
  }
  $sections = array();
  foreach ($data['documents'] as $document) {
    $sections[] = array('Wijzigingsvoorstellen – ' . $document['title'], $document['amendments']);
  }
  if ($data['motions']) {
    $sections[] = array('Moties', $data['motions']);
  }
  foreach ($sections as $index => $section) {
    list($title, $items) = $section;
    $blocks[] = array('page_break' => $index > 0) + ammo_docx_block($title, 'Heading2');
    $first = TRUE;
    foreach ($items as $item) {
      if (!$first) {
        $blocks[] = array('rule' => TRUE);
      }
      $first = FALSE;
      $is_motion = isset($item['consideration_body']);
      $blocks[] = ammo_docx_block(($is_motion ? 'Motie nr. ' : '') . $item['number'], 'Heading3');
      if (!$is_motion) {
        $blocks[] = array('runs' => array(array('text' => $item['location'], 'bold' => TRUE)));
      }
      $blocks[] = ammo_docx_label_block('Ingediend door:', $item['submitter']);
      if ($item['co_submitters'] !== '') {
        $blocks[] = ammo_docx_label_block('Mede-indieners:', $item['co_submitters']);
      }
      if ($is_motion) {
        $blocks[] = ammo_docx_label_block(ammo_docx_plain($item['consideration_opening']), $item['consideration_body']);
        $blocks[] = ammo_docx_label_block(ammo_docx_plain($item['follow_up_opening']), $item['follow_up_body']);
      }
      else {
        $blocks[] = array('runs' => array(array('text' => 'Wijzigingsvoorstel tekst', 'bold' => TRUE)));
        $blocks[] = ammo_docx_block($item['text']);
      }
      if (ammo_docx_plain($item['supplement']) !== '') {
        $blocks[] = array('runs' => array(array('text' => 'Toelichting', 'bold' => TRUE)));
        $blocks[] = ammo_docx_block($item['supplement']);
      }
      if ($item['state'] !== '') {
        $blocks[] = ammo_docx_label_block('Status:', $item['state']);
      }
      if ($item['advice'] !== '') {
        $blocks[] = ammo_docx_label_block('Advies:', $item['advice']);
        if (ammo_docx_plain($item['advice_supplement']) !== '') {
          $blocks[] = ammo_docx_block($item['advice_supplement']);
        }
      }
    }
  }
  return $blocks;
}

/**
 * The download link when the overview lists any items, keeping a branch chosen by an admin.
 */
function ammo_branch_overview_download_link($meeting_id, array $branches, $count) {
  if (!$count) {
    return array();
  }
  $link = array('title' => 'Download als Word-bestand (docx)', 'href' => 'ammo/afdeling/' . $meeting_id . '/download');
  if (isset($_GET['afdeling']) && is_scalar($_GET['afdeling']) && count($branches) === 1 && (string) key($branches) === (string) $_GET['afdeling']) {
    $link['query'] = array('afdeling' => key($branches));
  }
  return array('download' => $link);
}

/**
 * Template data: per branch a submitted and a co-submitted part with tables.
 */
function ammo_branch_overview_page($meeting_id, array $branches) {
  $page = array('branches' => array(), 'meeting_available' => FALSE, 'downloads' => array());
  $meeting = ammo_branch_overview_meeting($meeting_id);
  if (!$meeting) {
    return $page;
  }
  $page['meeting_available'] = TRUE;
  $types = array('amendment' => array('Wijzigingsvoorstellen', array('Nr', 'Locatie', 'Status', 'Advies')));
  if (ammo_branch_overview_shows_motions($meeting)) {
    $types['motion'] = array('Moties', array('Nr', 'Motie', 'Status', 'Advies'));
  }
  $counts = array_fill_keys(array_keys($types), 0);
  $parts = array('submitted' => array('Ingediend door', 'Mede-indieners'), 'co_submitted' => array('Mede ingediend door', 'Hoofdindiener'));
  foreach ($branches as $branch_id => $name) {
    $page['branches'][$branch_id] = array_fill_keys(array_keys($parts), array('tables' => array()));
  }
  foreach ($types as $type => $table) {
    list($caption, $header) = $table;
    $overview = ammo_branch_overview_items($type, $meeting->id, array_keys($branches));
    foreach ($overview as $branch_id => $branch_items) {
      foreach ($parts as $part => $labels) {
        $rows = array();
        $document_id = NULL;
        $counts[$type] += count($branch_items[$part]);
        foreach ($branch_items[$part] as $item) {
          // Amendments are grouped under a heading row per document.
          if ($type === 'amendment' && $item['document_id'] != $document_id) {
            $document_id = $item['document_id'];
            $rows[] = array(
              'data' => array(array('data' => check_plain($item['document_title']), 'header' => TRUE, 'colspan' => count($header) + 1, 'scope' => 'colgroup')),
              'class' => array('document'),
              'no_striping' => TRUE,
            );
          }
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
  $page['downloads'] = ammo_branch_overview_download_link($meeting->id, $branches, array_sum($counts));
  return $page;
}
