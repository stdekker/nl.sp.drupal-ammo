<?php
global $base_url;

// Format amendment ID with "X" fallback for empty chapter
$chapter_display = (!empty($chapter)) ? $chapter : 'X';
$amendment_id = (!empty($chapterized_id)) ? $chapter_display . '.' . $chapterized_id : $chapter_display . '.' . $entity_id;
$is_withdrawn = ($state === 'withdrawn');
?>
<h3 id="amendment<?php print $entity_id; ?>" class="amendment-header <?php print $is_withdrawn ? 'withdrawn-header' : ''; ?>">
  Voorstel <?php print $amendment_id; ?> (pagina <?php print $page; ?>, regel <?php print $line; ?>)
  <?php if ($is_withdrawn): ?>
    <span class="withdrawn-indicator"> - Ingetrokken</span>
  <?php endif; ?>
</h3>

<?php if ($is_withdrawn): ?>
<a href="#" class="withdrawn-toggle" data-target="amendment-content-<?php print $entity_id; ?>">[Toon details]</a>
<div id="amendment-content-<?php print $entity_id; ?>" class="amendment-content-collapsed" style="display: none;">
<?php endif; ?>

<?php
// Process owners - always use h4 heading for consistency
print '<h4>Indiener(s):</h4>';

// Process branch owners
if (!empty($owners_branch)) {
  $list = array();
  foreach ($owners_branch as $owner_branch) {
    $list[] = $owner_branch['contact_display_name'];
  }
  $last = array_pop($list);
  $owners_list = (count($list) === 0) ? $last : implode(', ', $list) . ' en ' . $last;
}

// Process member owners
if (!empty($owners_member)) {
  $list = array();
  foreach ($owners_member as $owner_member) {
    $list[] = $owner_member['contact_display_name'];
  }
  $last = array_pop($list);
  $members_list = (count($list) === 0) ? $last : implode(', ', $list) . ' en ' . $last;
  
  print '<p>Ingediend door ' . $members_list . '.</p>';
  $number = count($backers) + count($owners_member);
  
  if (!empty($owners_branch)) {
    print '<p>Mede ingediend door ' . $owners_list . '.</p>';
  }
  print '<p>Ondersteund door 50 leden (voldoende steun).</p>';
} else if (!empty($owners_branch)) {
  print '<p>' . $owners_list . '</p>';
}
?>

<h4>Voorstel:</h4>
<div class="text-limit"><?php print $amendment_text; ?></div>

<?php if (!empty($supplement)): ?>
<h4>Toelichting voorstel:</h4>
<div class="text-limit"><?php print $supplement; ?></div>
<?php endif; ?>

<?php if (!empty($state) && (!$hide_state || $state === 'withdrawn' || ($admin_access && empty($mail)))): ?>
<h4>Status:</h4>
<?php
  $options = ammo_states();
  print $options[$state];
  if (!empty($state_supplement)) {
    print '<h4>Toelichting status:</h4>';
    print '<p>' . $state_supplement . '</p>';
  }
?>
<?php endif; ?>

<?php
// Process vote requests - only show when there are actual vote requests
if (!empty($vote_requests) && is_array($vote_requests) && count($vote_requests) > 0) {
  print '<h4>Stemming aangevraagd door:</h4>';
  $vote_list = array();
  foreach ($vote_requests as $vote_request) {
    $vote_list[] = $vote_request['contact_display_name'];
  }
  print '<p>' . implode(', ', $vote_list) . '.</p>';
}
?>

<?php if ((!empty($advice)) && (!$hide_advice || ($admin_access && empty($mail)))): ?>
<h4>Advies:</h4>
<p>
<?php
  $options = ammo_amendment_advice();
  print $options[$advice];
  if (!empty($advice_supplement)) {
    print ' &ndash; ' . $advice_supplement;
  }
?>
</p>
<?php endif; ?>

<?php if (empty($no_links)): ?>
<?php $dest = (!empty($destination) ? $destination : ammo_get_destination()); ?>
<ul class="ammo-list">
  <?php if (!empty($owners_branch) || !empty($owners_member)): ?>
    <?php if ($edit_access): ?>
      <li>Bewerken:</li>
      <li><?php print l('inhoud', 'ammo/amendment/edit/' . $entity_id, array('query' => $dest)); ?></li>
    <?php endif; ?>
    <?php if (($admin_access && $support_access) || $superadmin_access): ?>
      <li><?php print l('status/advies', 'ammo/amendment/advice/' . $entity_id, array('query' => $dest)); ?></li>
    <?php endif; ?>
    <?php if ($admin_access): ?>
      <li><?php print l('indieners', 'ammo/amendment/ownership/' . $entity_id, array('query' => $dest)); ?></li>
      <?php if ($state !== 'spelling_grammar'): ?>
        <li><?php print l('stemming', 'ammo/amendment/vote-request/' . $entity_id, array('query' => $dest)); ?></li>
      <?php endif; ?>
    <?php endif; ?>
  <?php endif; ?>
  <?php if (!empty($owners_branch) || !empty($owners_member)): ?>
    <?php if ($withdraw_access && $state !== 'withdrawn' && $state !== 'spelling_grammar'): ?>
      <?php if ($unsupported_branches): ?>
        <li><?php print l('mede indienen', 'ammo/support/add/branch/amendment/' . $entity_id, array('query' => $dest)); ?></li>
      <?php endif; ?>
      <?php if ($supported_branches && (empty($vote_requests) || !is_array($vote_requests) || count($vote_requests) == 0)): ?>
        <li><?php print l('voorstel / steun intrekken', 'ammo/support/withdraw/branch/amendment/' . $entity_id, array('query' => $dest)); ?></li>
      <?php endif; ?>
      <?php if ($removable_member_owners): ?>
        <li><?php print l('intrekken individuele indieners', 'ammo/support/withdraw/branchmembers/amendment/' . $entity_id, array('query' => $dest)); ?></li>
      <?php endif; ?>
    <?php endif; ?>
    <?php if ($vote_request_access): ?>
      <?php if ($vote_unrequested_branches): ?>
        <li><?php print l('stemming aanvragen', 'ammo/vote_request/add/branch/amendment/' . $entity_id, array('query' => $dest)); ?></li>
      <?php endif; ?>
      <?php if ($vote_requested_branches): ?>
        <li><?php print l('stemming intrekken', 'ammo/vote_request/withdraw/branch/amendment/' . $entity_id, array('query' => $dest)); ?></li>
      <?php endif; ?>
    <?php endif; ?>
  <?php endif; ?>
</ul>
<?php endif; ?>

<?php if ($is_withdrawn): ?>
</div>
<?php endif; ?>
