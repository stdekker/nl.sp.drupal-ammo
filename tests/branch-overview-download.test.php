<?php

/** Run with ddev drush scr; fixtures are rolled back. */
module_load_include('inc', 'ammo', 'ammo');
module_load_include('inc', 'ammo', 'branch-overview');
module_load_include('inc', 'ammo', 'docx');

function ammo_branch_download_assert($condition, $message) {
  if (!$condition) {
    throw new Exception($message);
  }
}

function ammo_branch_download_owner($type, $id, $contact_id, $name) {
  ammo_save_entity('contact_relation', array('target_id' => $id, 'target_type' => $type, 'relation_type' => 'owner', 'contact_type' => 'branch', 'contact_id' => $contact_id, 'branch_id' => $contact_id, 'contact_display_name' => $name, 'branch_display_name' => $name));
}

/** The document XML of the download as the download page builds it, after checking the file is a valid Word package. */
function ammo_branch_download_xml($meeting, array $branches) {
  $bytes = ammo_docx_build(ammo_branch_overview_download_blocks(ammo_branch_overview_download_data(entity_load_single('meeting', $meeting->id), $branches)), ammo_branch_overview_download_footer(REQUEST_TIME));
  ammo_branch_download_assert(is_string($bytes) && substr($bytes, 0, 2) === 'PK', 'The download is a zip package');
  $path = drupal_tempnam('temporary://', 'ammo-test-');
  file_put_contents($path, $bytes);
  $zip = new ZipArchive();
  $zip->open(drupal_realpath($path));
  $parts = array();
  foreach (array('[Content_Types].xml', '_rels/.rels', 'word/_rels/document.xml.rels', 'word/styles.xml', 'word/footer1.xml', 'word/document.xml') as $name) {
    $parts[$name] = $zip->getFromName($name);
    ammo_branch_download_assert($parts[$name] !== FALSE && simplexml_load_string($parts[$name]) !== FALSE, 'Well-formed part: ' . $name);
  }
  $zip->close();
  drupal_unlink($path);
  ammo_branch_download_assert(strpos($parts['word/footer1.xml'], '>' . ammo_branch_overview_download_footer(REQUEST_TIME) . '<') !== FALSE && strpos($parts['word/document.xml'], '<w:footerReference w:type="default" r:id="rId2"/>') !== FALSE && strpos($parts['word/_rels/document.xml.rels'], 'Target="footer1.xml"') !== FALSE, 'Footer with the download time on every page');
  return $parts['word/document.xml'];
}

/** The text of the document, one paragraph per line. */
function ammo_branch_download_output($meeting, array $branches) {
  $xml = ammo_branch_download_xml($meeting, $branches);
  return html_entity_decode(strip_tags(str_replace(array('</w:p>', '<w:br/>'), "\n", $xml)), ENT_QUOTES | ENT_XML1, 'UTF-8');
}

global $user;
$previous_user = $user;
$previous_get = $_GET;
$a = 99999901;
$b = 99999902;
$transaction = db_transaction();
try {
  $user = user_save(NULL, array('name' => 'ammo-branch-download-' . uniqid(), 'status' => 1, 'field_contact_id' => array(LANGUAGE_NONE => array(array('value' => 99999999)))));
  $permissions = &drupal_static('user_access');
  $permissions[$user->uid] = array('add amendments and motions' => TRUE);
  $afdelingen = &drupal_static('ammo_get_afdelingen');
  $afdelingen[99999999] = array($a => 'SP Afdeling A');
  $afdelingen['all'] = array($a => 'SP Afdeling A', $b => 'SP Afdeling B');
  $branches = array($a => 'SP Afdeling A');

  $meeting = ammo_save_entity('meeting', array('title' => 'Branch download test', 'date' => '2099-01-01', 'withdrawal_closing_date' => '2099-01-01', 'hide_meeting' => 0, 'hide_state' => 0, 'hide_advice' => 0, 'motion_submission_end' => '2099-01-01'));
  $first = ammo_save_entity('document', array('title' => 'A eerste document', 'meeting_id' => $meeting->id));
  $second = ammo_save_entity('document', array('title' => 'B tweede document', 'meeting_id' => $meeting->id));
  ammo_save_entity('document', array('title' => 'C leeg document', 'meeting_id' => $meeting->id));
  $fixtures = array(
    'own' => array($first, 'submitted', 'accept', 'Tekst eigen voorstel'),
    'co' => array($first, 'assessed', 'reject', 'Tekst mede ingediend'),
    'second' => array($second, 'submitted', '', 'Tekst tweede document'),
    'draft_other' => array($first, 'concept', '', 'Tekst concept van B'),
    'blocked' => array($first, 'blocked', '', 'Tekst verwijderd voorstel'),
    'unrelated' => array($first, 'submitted', '', 'Tekst van een ander'),
  );
  $ids = array();
  foreach ($fixtures as $key => $values) {
    list($document, $state, $advice, $text) = $values;
    $entry = ammo_save_entity('amendment', array('document_id' => $document->id, 'chapter' => 1, 'chapterized_id' => count($ids) + 1, 'page' => 2, 'line' => 3, 'state' => $state, 'advice' => $advice, 'advice_supplement' => $advice ? 'Toelichting ' . $key : '', 'amendment_text' => $text));
    $ids[$key] = (int) $entry->id;
  }
  ammo_branch_download_owner('amendment', $ids['own'], $a, 'Afdeling A');
  ammo_branch_download_owner('amendment', $ids['own'], $b, 'Afdeling B');
  ammo_branch_download_owner('amendment', $ids['co'], $b, 'Afdeling B');
  ammo_branch_download_owner('amendment', $ids['co'], $a, 'Afdeling A');
  ammo_branch_download_owner('amendment', $ids['second'], $a, 'Afdeling A');
  ammo_branch_download_owner('amendment', $ids['draft_other'], $b, 'Afdeling B');
  ammo_branch_download_owner('amendment', $ids['draft_other'], $a, 'Afdeling A');
  ammo_branch_download_owner('amendment', $ids['blocked'], $a, 'Afdeling A');
  ammo_branch_download_owner('amendment', $ids['unrelated'], $b, 'Afdeling B');
  $motion = ammo_save_entity('motion', array('meeting_id' => $meeting->id, 'motion_id' => 7, 'state' => 'assessed', 'advice' => 'accept', 'consideration_body' => 'Overweging van de motie'));
  ammo_branch_download_owner('motion', $motion->id, $a, 'Afdeling A');
  $second_motion = ammo_save_entity('motion', array('meeting_id' => $meeting->id, 'motion_id' => 8, 'state' => 'submitted', 'consideration_body' => 'Overweging tweede motie'));
  ammo_branch_download_owner('motion', $second_motion->id, $b, 'Afdeling B');
  ammo_branch_download_owner('motion', $second_motion->id, $a, 'Afdeling A');
  $other_motion = ammo_save_entity('motion', array('meeting_id' => $meeting->id, 'motion_id' => 9, 'state' => 'submitted', 'consideration_body' => 'Overweging van een ander'));
  ammo_branch_download_owner('motion', $other_motion->id, $b, 'Afdeling B');

  // The download holds what the overview lists: submitted and co-submitted, nothing else.
  $output = ammo_branch_download_output($meeting, $branches);
  foreach (array('Tekst eigen voorstel', 'Tekst mede ingediend', 'Tekst tweede document', 'Overweging van de motie', 'Overweging tweede motie') as $text) {
    ammo_branch_download_assert(strpos($output, $text) !== FALSE, 'In the download: ' . $text);
  }
  foreach (array('Tekst concept van B', 'Tekst verwijderd voorstel', 'Tekst van een ander', 'Overweging van een ander', 'C leeg document') as $text) {
    ammo_branch_download_assert(strpos($output, $text) === FALSE, 'Not in the download: ' . $text);
  }

  // Each document and the motions get their own heading, with a line between the proposals of a section.
  $xml = ammo_branch_download_xml($meeting, $branches);
  ammo_branch_download_assert(preg_match_all('/<w:pStyle w:val="Heading2"\/>.*?<w:t xml:space="preserve">([^<]*)<\/w:t>/', $xml, $headings) === 3 && $headings[1] === array('Wijzigingsvoorstellen – A eerste document', 'Wijzigingsvoorstellen – B tweede document', 'Moties'), 'A heading per document, then Moties');
  ammo_branch_download_assert(substr_count($xml, '<w:pageBreakBefore/>') === 2, 'Every section after the first starts on a new page');
  ammo_branch_download_assert(substr_count($xml, '<w:pBdr>') === 2, 'A line between proposals within a section only: one in the first document, one between the motions');
  ammo_branch_download_assert(strpos($output, 'Tekst eigen voorstel') < strpos($output, 'Tekst mede ingediend') && strpos($output, 'Tekst mede ingediend') < strpos($output, 'Tekst tweede document'), 'Amendments sorted within their document');
  ammo_branch_download_assert(strpos($output, "Ingediend door: Afdeling A\n") !== FALSE && strpos($output, "Mede-indieners: Afdeling B\n") !== FALSE && strpos($output, "Ingediend door: Afdeling B\n") !== FALSE, 'Submitter and co-submitters per proposal');
  ammo_branch_download_assert(strpos($output, "Branch download test\nVoorstellen van Afdeling A.\n") === 0, 'Meeting title and afdeling on top');
  ammo_branch_download_assert(preg_match('/^Gedownload op \d{1,2} \S+ \d{4} om \d{2}:\d{2}$/u', ammo_branch_overview_download_footer(REQUEST_TIME)), 'Footer gives the date and time of the download');
  ammo_branch_download_assert(strpos(ammo_docx_paragraph(ammo_docx_block("A & <b>B</b>\r\nC")), '<w:t xml:space="preserve">A &amp; B</w:t><w:br/><w:t xml:space="preserve">C</w:t>') !== FALSE, 'Text is escaped, tags removed and lines kept');

  // Advice only once assessed; hidden statuses and advice stay hidden, as on the overview.
  ammo_branch_download_assert(strpos($output, 'Toelichting own') === FALSE && strpos($output, 'Advies: Afwijzen') !== FALSE && strpos($output, 'Toelichting co') !== FALSE && strpos($output, 'Advies: Overnemen') !== FALSE, 'Advice left out while Ingediend, shown once assessed');
  $meeting->hide_state = 1;
  $meeting->hide_advice = 1;
  entity_save('meeting', $meeting);
  $output = ammo_branch_download_output($meeting, $branches);
  ammo_branch_download_assert(strpos($output, 'Status:') === FALSE && strpos($output, 'Advies:') === FALSE && strpos($output, 'Toelichting co') === FALSE, 'Hidden status and advice are not in the download');

  // One link, keeping an admin's chosen branch, and none without items.
  $page = ammo_branch_overview_page($meeting->id, $branches);
  ammo_branch_download_assert(array_keys($page['downloads']) === array('download') && $page['downloads']['download']['href'] === 'ammo/afdeling/' . $meeting->id . '/download' && !isset($page['downloads']['download']['query']), 'One download link');
  $markup = theme('overview_branch', array('meeting_id' => $meeting->id, 'selected_branches' => $branches));
  ammo_branch_download_assert(strpos($markup, 'Download als Word-bestand (docx)') !== FALSE, 'Overview shows the download link');
  ammo_branch_download_assert(ammo_branch_overview_download_link($meeting->id, $branches, 0) === array(), 'No link without items');
  $_GET['afdeling'] = (string) $b;
  $link = ammo_branch_overview_download_link($meeting->id, array($b => 'SP Afdeling B'), 1);
  ammo_branch_download_assert($link['download']['query'] === array('afdeling' => $b), 'Chosen branch is kept in the link');
  $_GET = $previous_get;

  // Without motions for the meeting there is no Moties part.
  $meeting->motion_submission_end = NULL;
  entity_save('meeting', $meeting);
  $output = ammo_branch_download_output($meeting, $branches);
  $xml = ammo_branch_download_xml($meeting, $branches);
  ammo_branch_download_assert(strpos($output, "\nMoties\n") === FALSE && strpos($output, 'Overweging van de motie') === FALSE && substr_count($xml, '<w:pBdr>') === 1, 'Motions left out when the meeting has none');

  ammo_branch_download_assert(ammo_branch_overview_download('999999999') === MENU_NOT_FOUND && ammo_branch_overview_download('x') === MENU_NOT_FOUND, 'Unknown or invalid meeting is not found');
  $meeting->hide_meeting = 1;
  entity_save('meeting', $meeting);
  ammo_branch_download_assert(ammo_branch_overview_download($meeting->id) === MENU_NOT_FOUND, 'Hidden meeting is not found');

  // The admin print still lists every item of the chosen states, with status and advice.
  $output = theme('print_amendments_by_location', array('values' => array('meeting' => $meeting->id, 'status' => array('submitted' => 'submitted', 'concept' => 'concept'), 'vote_requested_only' => 0, 'notespace' => 0, 'pagebreaks' => 0)));
  ammo_branch_download_assert(strpos($output, 'Tekst eigen voorstel') !== FALSE && strpos($output, 'Tekst concept van B') !== FALSE && strpos($output, 'Tekst van een ander') !== FALSE && strpos($output, 'Tekst mede ingediend') === FALSE && strpos($output, 'Advies:</strong> Overnemen') !== FALSE, 'Admin print is unchanged');

  $transaction->rollback();
  print "PASS: one valid Word file with the download time in its footer and the submitted and co-submitted items of the branch only, a heading per document and for the motions, lines between proposals, motions only when the meeting has them, status and advice hidden as on Mijn afdeling, one link with the chosen branch, not found for unknown or hidden meetings, admin print unchanged. Fixtures rolled back.\n";
}
catch (Throwable $e) {
  $transaction->rollback();
  throw $e;
}
finally {
  $user = $previous_user;
  $_GET = $previous_get;
}
