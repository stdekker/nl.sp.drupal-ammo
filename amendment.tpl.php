<?php
global $base_url;

// Format amendment ID with "X" fallback for empty chapter
$chapter_display = (!empty($chapter)) ? $chapter : 'X';
$amendment_id = (!empty($chapterized_id)) ? $chapter_display . '.' . $chapterized_id : $chapter_display . '.' . $entity_id;
?>
<h3 id="amendment<?php print $entity_id; ?>"><?php print $amendment_id; ?> (pagina <?php print $page; ?>, regel <?php print $line; ?>)</h3>

<?php
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
  print '<h4>Indiener(s):</h4>';
  print '<p>' . $owners_list . '.</p>';
}
?>

<h4>Voorstel:</h4>
<p class="text-limit"><?php print $amendment_text; ?></p>

<?php if (!empty($supplement)): ?>
<h4>Toelichting:</h4>
<p class="text-limit"><?php print $supplement; ?></p>
<?php endif; ?>

<?php if (!empty($state) || !empty($advice)): ?>
<p>
<?php
if (!empty($state) && (!$hide_state || $state === 'withdrawn' || ($admin_access && empty($mail)))) {
  $options = ammo_states();
  print '<strong>Status:</strong> ' . strtolower($options[$state]);
  if (!empty($state_supplement)) {
    print '<br/>' . $state_supplement;
  }
}

if (!empty($advice) && (!$hide_advice || ($admin_access && empty($mail)))) {
  if (!empty($state)) {
    print '<br/>';
  }
  $options = ammo_amendment_advice();
  print '<strong>Advies:</strong> ' . strtolower($options[$advice]);
  if (!empty($advice_supplement)) {
    print '<br/>' . $advice_supplement;
  }
}
?>
</p>
<?php endif; ?>

<?php if (empty($no_links)): ?>
<?php $dest = (!empty($destination) ? $destination : ammo_get_destination()); ?>
<ul class="ammo-list">
  <?php if (!empty($owners_branch) || !empty($owners_member)): ?>
    <?php if ($edit_access): ?>
      <li><?php print l('bewerk voorstel', 'ammo/amendment/edit/' . $entity_id, array('query' => $dest)); ?></li>
    <?php endif; ?>
    <?php if (($admin_access && $support_access) || $superadmin_access): ?>
      <li><?php print l('bewerk advies', 'ammo/amendment/advice/' . $entity_id, array('query' => $dest)); ?></li>
    <?php endif; ?>
  <?php endif; ?>
  <?php if (!empty($owners_branch) || !empty($owners_member)): ?>
    <?php if ($withdraw_access): ?>
      <?php if ($unsupported_branches): ?>
        <li><?php print l('mede indienen als afdeling', 'ammo/support/add/branch/amendment/' . $entity_id, array('query' => $dest)); ?></li>
      <?php endif; ?>
      <?php if ($supported_branches): ?>
        <li><?php print l('intrekken als afdeling', 'ammo/support/withdraw/branch/amendment/' . $entity_id, array('query' => $dest)); ?></li>
      <?php endif; ?>
      <?php if ($removable_member_owners): ?>
        <li><?php print l('intrekken individuele indieners', 'ammo/support/withdraw/branchmembers/amendment/' . $entity_id, array('query' => $dest)); ?></li>
      <?php endif; ?>
    <?php endif; ?>
  <?php endif; ?>
</ul>
<?php endif; ?>
