<?php

/** Run with ddev drush scr; fixtures are rolled back. */
module_load_include('inc', 'ammo', 'ammo');
module_load_include('inc', 'ammo', 'branch-overview');
module_load_include('inc', 'ammo', 'branch-overview.form');

function ammo_branch_overview_assert($condition, $message) {
  if (!$condition) {
    throw new Exception($message);
  }
}

function ammo_branch_overview_test_owner($type, $id, $contact_id, $name, $contact_type = 'branch', $relation_type = 'owner') {
  ammo_save_entity('contact_relation', array('target_id' => $id, 'target_type' => $type, 'relation_type' => $relation_type, 'contact_type' => $contact_type, 'contact_id' => $contact_id, 'branch_id' => $contact_id, 'contact_display_name' => $name, 'branch_display_name' => $name));
}

global $user;
$previous_user = $user;
$previous_get = $_GET;
$a = 99999901;
$b = 99999902;
$transaction = db_transaction();
try {
  $user = user_save(NULL, array('name' => 'ammo-branch-overview-' . uniqid(), 'status' => 1, 'field_contact_id' => array(LANGUAGE_NONE => array(array('value' => 99999999)))));
  $member = $user;
  $permissions = &drupal_static('user_access');
  $permissions[$user->uid] = array('add amendments and motions' => TRUE);
  $branches = &drupal_static('ammo_get_afdelingen');
  $branches[99999999] = array($a => 'SP Afdeling A');
  $branches['all'] = array($a => 'SP Afdeling A', $b => 'SP Afdeling B');

  $meeting = ammo_save_entity('meeting', array('title' => 'Branch overview test', 'date' => '2099-01-01', 'amendment_withdraw_end' => '2099-01-01', 'motion_withdraw_end' => '2099-01-01', 'amendment_support_withdraw_end' => '2099-01-01', 'motion_support_withdraw_end' => '2099-01-01', 'amendment_vote_request_end' => '2099-01-01', 'motion_vote_request_end' => '2099-01-01', 'hide_meeting' => 0, 'hide_state' => 0, 'motion_submission_end' => '2099-01-01'));
  $document = ammo_save_entity('document', array('title' => 'Branch overview document', 'meeting_id' => $meeting->id));
  $ids = array();
  foreach (array('shared' => 'submitted', 'admin_first' => 'submitted', 'member_first' => 'submitted', 'vote_only' => 'submitted', 'second' => 'submitted', 'draft' => 'concept', 'blocked' => 'blocked', 'rejected' => 'assessed') as $key => $state) {
    $entry = ammo_save_entity('amendment', array('document_id' => $document->id, 'chapter' => 2, 'chapterized_id' => count($ids) + 1, 'page' => 14, 'line' => 220, 'state' => $state));
    $ids[$key] = (int) $entry->id;
  }
  ammo_branch_overview_test_owner('amendment', $ids['shared'], $a, 'Afdeling A');
  ammo_branch_overview_test_owner('amendment', $ids['shared'], $b, 'Afdeling B');
  ammo_branch_overview_test_owner('amendment', $ids['admin_first'], 0, 'de congrescommissie');
  ammo_branch_overview_test_owner('amendment', $ids['admin_first'], $b, 'Afdeling B');
  ammo_branch_overview_test_owner('amendment', $ids['member_first'], $b, 'Afdeling B');
  ammo_branch_overview_test_owner('amendment', $ids['member_first'], 99999999, 'Lid Een', 'member');
  ammo_branch_overview_test_owner('amendment', $ids['vote_only'], $a, 'Afdeling A', 'branch', 'vote_request');
  ammo_branch_overview_test_owner('amendment', $ids['second'], $a, 'Afdeling A');
  ammo_branch_overview_test_owner('amendment', $ids['second'], $b, 'Afdeling B');
  ammo_branch_overview_test_owner('amendment', $ids['draft'], $a, 'Afdeling A');
  ammo_branch_overview_test_owner('amendment', $ids['draft'], $b, 'Afdeling B');
  ammo_branch_overview_test_owner('amendment', $ids['blocked'], $a, 'Afdeling A');
  ammo_branch_overview_test_owner('amendment', $ids['rejected'], $a, 'Afdeling A');
  $motion = ammo_save_entity('motion', array('meeting_id' => $meeting->id, 'motion_id' => 7, 'state' => 'submitted', 'consideration_body' => 'Overweging van de motie'));
  ammo_branch_overview_test_owner('motion', $motion->id, $b, 'Afdeling B');
  ammo_branch_overview_test_owner('motion', $motion->id, $a, 'Afdeling A');

  $items = ammo_branch_overview_items('amendment', $meeting->id, array($a, $b));
  ammo_branch_overview_assert(array_keys($items[$a]['submitted']) === array($ids['shared'], $ids['second'], $ids['draft'], $ids['rejected']), 'First owner sees own items, drafts included, without blocked or vote-only items');
  ammo_branch_overview_assert(!$items[$a]['co_submitted'], 'First owner is not its own co-submitter');
  ammo_branch_overview_assert(!$items[$b]['submitted'], 'Later, admin-preceded and member-preceded owners did not submit');
  ammo_branch_overview_assert(array_keys($items[$b]['co_submitted']) === array($ids['shared'], $ids['admin_first'], $ids['member_first'], $ids['second']), 'Co-submitter sees shared items but not another branch draft');
  ammo_branch_overview_assert($items[$a]['submitted'][$ids['second']]['others'] === array($b => 'Afdeling B'), 'Co-submitters listed without the branch itself');
  ammo_branch_overview_assert($items[$b]['co_submitted'][$ids['admin_first']]['others'] === array('de congrescommissie'), 'Admin owner is the first owner');
  ammo_branch_overview_assert($items[$b]['co_submitted'][$ids['member_first']]['others'] === array('Lid Een'), 'Member owner precedes an earlier branch owner');
  $motions = ammo_branch_overview_items('motion', $meeting->id, array($a));
  ammo_branch_overview_assert(array_keys($motions[$a]['co_submitted']) === array((int) $motion->id) && !$motions[$a]['submitted'], 'Motions follow the same rule');

  ammo_branch_overview_assert(ammo_branch_overview_branches(array('afdeling' => $b)) === array($a => 'SP Afdeling A'), 'Regular user cannot select another branch');
  $page = ammo_branch_overview_page($meeting->id, ammo_branch_overview_branches(array()));
  $tables = $page['branches'][$a]['submitted']['tables'];
  ammo_branch_overview_assert($page['branches'][$a]['submitted']['title'] === 'Ingediend door Afdeling A', 'Branch name cleaned in heading');
  ammo_branch_overview_assert(isset($tables['motion']) && $page['branches'][$a]['co_submitted']['tables']['motion']['rows'], 'Motions listed when the meeting has them');
  $document_row = $tables['amendment']['rows'][0];
  ammo_branch_overview_assert($document_row['class'] === array('document') && $document_row['data'][0]['data'] === 'Branch overview document' && $document_row['data'][0]['colspan'] === 5, 'Amendments grouped under a document heading row');
  $shared = $tables['amendment']['rows'][1];
  ammo_branch_overview_assert(strpos($shared['data'][0], 'ammo/amendment/view/' . $ids['shared'] . '">2.1</a>') !== FALSE, 'Amendment number links to its page');
  ammo_branch_overview_assert(count($shared['data']) === 5 && $shared['data'][1] === '2 / 14 / 220' && $shared['data'][2] === 'Ingediend' && $shared['data'][3] === '' && $shared['data'][4] === 'Afdeling B', 'All locators, status, empty advice and co-submitter shown, without Document and Groep columns');
  $draft = $tables['amendment']['rows'][3];
  ammo_branch_overview_assert($draft['data'][2] === 'Concept' && strpos($draft['data'][0], 'ammo/amendment/view/' . $ids['draft'] . '"') !== FALSE, 'Draft labelled and linked to its page');
  $motion_row = $page['branches'][$a]['co_submitted']['tables']['motion']['rows'][0];
  ammo_branch_overview_assert(strpos($motion_row['data'][0], 'ammo/motion/view/' . $motion->id . '">7</a>') !== FALSE && $motion_row['data'][4] === 'Afdeling B', 'Motion link and first submitter shown');
  $markup = theme('overview_branch', array('meeting_id' => $meeting->id, 'selected_branches' => array($a => 'SP Afdeling A')));
  ammo_branch_overview_assert(strpos($markup, '<h2>Ingediend door Afdeling A</h2>') < strpos($markup, '<h2>Mede ingediend door Afdeling A</h2>') && strpos($markup, 'Geen wijzigingsvoorstellen.') !== FALSE, 'Template renders both parts in order with empty text');
  ammo_branch_overview_assert(strpos(theme('overview_branch', array('meeting_id' => $meeting->id, 'selected_branches' => array())), 'geen afdeling') !== FALSE, 'User without branch gets a message');
  $state = array();
  $form = ammo_branch_overview_form(array(), $state, $meeting->id);
  ammo_branch_overview_assert(!isset($form['afdeling']) && !isset($form['submit']), 'Regular user has no branch selector');

  $meeting->hide_state = 1;
  $meeting->motion_submission_end = NULL;
  entity_save('meeting', $meeting);
  $page = ammo_branch_overview_page($meeting->id, array($a => 'SP Afdeling A'));
  $rows = $page['branches'][$a]['submitted']['tables']['amendment']['rows'];
  ammo_branch_overview_assert($rows[3]['data'][2] === 'Concept', 'Draft stays listed after the closing date');
  ammo_branch_overview_assert($rows[4]['data'][2] === '' && !$rows[4]['class'], 'Hidden state is neither printed nor used as class');
  ammo_branch_overview_assert(!isset($page['branches'][$a]['submitted']['tables']['motion']), 'No motions part for a meeting without motions');
  $rejected = entity_load_single('amendment', $ids['rejected']);
  $rejected->advice = 'reject';
  entity_save('amendment', $rejected);
  $rows = ammo_branch_overview_page($meeting->id, array($a => 'SP Afdeling A'))['branches'][$a]['submitted']['tables']['amendment']['rows'];
  ammo_branch_overview_assert($rows[4]['data'][3] === 'Afwijzen', 'Advice shown');
  $meeting->hide_advice = 1;
  entity_save('meeting', $meeting);
  $rows = ammo_branch_overview_page($meeting->id, array($a => 'SP Afdeling A'))['branches'][$a]['submitted']['tables']['amendment']['rows'];
  ammo_branch_overview_assert($rows[4]['data'][3] === '', 'Hidden advice is not printed');
  $later = ammo_save_entity('document', array('title' => 'Z later document', 'meeting_id' => $meeting->id));
  $later_entry = ammo_save_entity('amendment', array('document_id' => $later->id, 'chapter' => 1, 'chapterized_id' => 1, 'page' => 3, 'line' => 4, 'state' => 'submitted'));
  ammo_branch_overview_test_owner('amendment', $later_entry->id, $a, 'Afdeling A');
  $rows = ammo_branch_overview_page($meeting->id, array($a => 'SP Afdeling A'))['branches'][$a]['submitted']['tables']['amendment']['rows'];
  ammo_branch_overview_assert(count($rows) === 7 && $rows[5]['data'][0]['data'] === 'Z later document' && strpos($rows[6]['data'][0], 'ammo/amendment/view/' . $later_entry->id . '"') !== FALSE, 'Each document gets its own heading above its amendments');
  ammo_branch_overview_assert(ammo_branch_overview_state_label('motion', 'assessed', $meeting, 'reject') === 'Beoordeeld' && ammo_branch_overview_state_label('motion', 'assessed', $meeting, 'accept') === '', 'Motion assessed as rejected stays visible when states are hidden');

  $user = user_load(1);
  ammo_branch_overview_assert(ammo_branch_overview_branches(array('afdeling' => $b)) === array((string) $b => 'SP Afdeling B'), 'Administrator can select a branch');
  ammo_branch_overview_assert(ammo_branch_overview_branches(array('afdeling' => array($b))) === ammo_branch_overview_branches(array('afdeling' => 1)), 'Invalid selection falls back to own branches');
  $admin_items = ammo_branch_overview_items('amendment', $meeting->id, array($a));
  ammo_branch_overview_assert(isset($admin_items[$a]['submitted'][$ids['blocked']]), 'Administrator sees blocked items');
  $admin_page = ammo_branch_overview_page($meeting->id, array($a => 'SP Afdeling A'));
  $admin_rows = $admin_page['branches'][$a]['submitted']['tables']['amendment']['rows'];
  ammo_branch_overview_assert($admin_rows[4]['data'][2] === 'Verwijderd' && $admin_rows[5]['data'][2] === 'Beoordeeld' && $admin_rows[5]['data'][3] === 'Afwijzen', 'Administrator sees blocked and hidden states and hidden advice');
  $_GET['afdeling'] = $b;
  $state = array();
  $form = ammo_branch_overview_form(array(), $state, $meeting->id);
  ammo_branch_overview_assert($form['afdeling']['#default_value'] === 'SP Afdeling B' && $form['afdeling']['#autocomplete_path'] === 'ammo/afdelingen/autocomplete' && isset($form['submit']), 'Administrator gets the branch autocomplete');

  // The afdeling autocomplete suggests for the last typed name and maps names back to ids.
  ob_start();
  ammo_afdeling_autocomplete('SP Afdeling A, afdeling');
  $matches = drupal_json_decode(ob_get_clean());
  ammo_branch_overview_assert($matches === array('SP Afdeling A, SP Afdeling B' => 'SP Afdeling B'), 'Autocomplete suggests the other afdeling after the typed one');
  $element = array('#parents' => array('afdelingen'), '#title' => 'Afdelingen', '#value' => 'sp afdeling b, SP Afdeling A') + ammo_afdeling_autocomplete_element('Afdelingen', '', array(), TRUE);
  $validated = array('values' => array());
  ammo_afdeling_autocomplete_validate($element, $validated);
  ammo_branch_overview_assert($validated['values']['afdelingen'] == array($b => $b, $a => $a) && !form_get_errors(), 'Multiple names map to id => id');
  $element['#ammo_multiple'] = FALSE;
  ammo_afdeling_autocomplete_validate($element, $validated);
  ammo_branch_overview_assert(form_get_errors(), 'A single field refuses two names');
  form_clear_error();
  $element['#value'] = 'SP Onbekend';
  ammo_afdeling_autocomplete_validate($element, $validated);
  ammo_branch_overview_assert(form_get_errors(), 'An unknown name is refused');
  form_clear_error();
  $element['#value'] = '';
  $element['#ammo_empty_value'] = 0;
  ammo_afdeling_autocomplete_validate($element, $validated);
  ammo_branch_overview_assert($validated['values']['afdelingen'] === 0 && !form_get_errors(), 'An empty single field gives its empty value');
  drupal_get_messages();
  $state = array('values' => array('meeting' => $meeting->id, 'afdeling' => $b));
  ammo_branch_overview_form_submit(array(), $state);
  ammo_branch_overview_assert($state['redirect'] === array('ammo/afdeling/' . $meeting->id, array('query' => array('afdeling' => $b))), 'Selection redirects with the branch');
  $_GET = $previous_get;

  $user = $member;
  $meeting->hide_meeting = 1;
  entity_save('meeting', $meeting);
  $hidden = ammo_branch_overview_page($meeting->id, array($a => 'SP Afdeling A'));
  ammo_branch_overview_assert(!$hidden['meeting_available'] && !$hidden['branches'], 'Hidden meeting lists nothing');
  ammo_branch_overview_assert(!ammo_branch_overview_page('invalid', array($a => 'SP Afdeling A'))['meeting_available'], 'Unknown meeting handled');
  $transaction->rollback();
  print "PASS: first owner and co-submitter split, member and admin precedence, drafts, blocked and hidden states, links, motions, hidden meeting, branch selection and the afdeling autocomplete. Fixtures rolled back.\n";
}
catch (Throwable $e) {
  $transaction->rollback();
  throw $e;
}
finally {
  $user = $previous_user;
  $_GET = $previous_get;
}
