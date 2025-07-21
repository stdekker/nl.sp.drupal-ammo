<?php 
global $base_url; 
$is_withdrawn = ($state === 'withdrawn');
?>
<h3 id="motion<?php print $entity_id; ?>" class="motion-header <?php print $is_withdrawn ? 'withdrawn-header' : ''; ?>">
  Motie <?php print (!empty($motion_id)) ? $motion_id : $entity_id; ?>
  <?php if ($is_withdrawn): ?>
    <span class="withdrawn-indicator"> - Ingetrokken</span>
  <?php endif; ?>
</h3>

<?php if ($is_withdrawn): ?>
<a href="#" class="withdrawn-toggle" data-target="motion-content-<?php print $entity_id; ?>">[Toon details]</a>
<div id="motion-content-<?php print $entity_id; ?>" class="motion-content-collapsed" style="display: none;">
<?php endif; ?>
<?php //print '<p>' . $meeting_title . ' van de SP, in vergadering bijeen op ' . $meeting_date . '.</p>'; ?>

<h4>Indiener(s):</h4>
<div class="ammo-section-content">
<?php if (!empty($owners_branch)) : ?>
  <?php $list = array(); ?>
  <?php foreach ($owners_branch as $owner_branch) : $list[] = $owner_branch['contact_display_name']; endforeach; ?>
  <?php $last = array_pop($list); ?>
  <?php if (count($list) === 0) : $owners_list = $last; else : $owners_list = implode(', ', $list) . ' en ' . $last; endif; ?>
<?php endif; ?>
<?php if (!empty($owners_member)) : ?>
  <?php $list = array(); ?>
  <?php foreach ($owners_member as $owner_member) : $list[] = $owner_member['contact_display_name']; endforeach; ?>
  <?php $last = array_pop($list); ?>
  <?php if (count($list) === 0) : $members_list = $last; else : $members_list = implode(', ', $list) . ' en ' . $last; endif; ?>
  <p><?php print $members_list; ?>.</p>
  <?php $number = count($backers) + count($owners_member); ?>
  <?php if (!empty($owners_branch)) : ?>
    <p>Ondersteund door <?php print $owners_list; ?>.</p>
  <?php endif; ?>
	<p>Ondersteund door 50 leden (voldoende steun).</p>
<?php else: ?>
  <?php if (!empty($owners_branch)) : ?>
    <p><?php print $owners_list; ?></p>
  <?php endif; ?>
<?php endif; ?>
</div>

<h4><?php print $consideration_opening; ?></h4>
<div class="ammo-section-content">
  <p class="text-limit"><?php print $consideration_body; ?></p>
</div>

<h4><?php print $follow_up_opening; ?></h4>
<div class="ammo-section-content">
  <p class="text-limit"><?php print $follow_up_body; ?></p>
</div>

<?php if (!empty($supplement)) : ?>
<h4>Toelichting:</h4>
<div class="ammo-section-content">
  <p class="text-limit"><?php print $supplement; ?></p>
</div>
<?php endif; ?>

<?php if (!empty($state) && (!$hide_state || $state === 'withdrawn' || ($admin_access && empty($mail)))) : ?>
<h4>Status:</h4>
<div class="ammo-section-content">
<p>
  <?php $options = ammo_states(); ?>
  <?php print ucfirst(strtolower($options[$state])); ?>
  <?php if (!empty($state_supplement)) : ?>
    <br/><?php print $state_supplement; ?>
  <?php endif; ?>
</p>
</div>
<?php endif; ?>

<?php if (!empty($advice) && (!$hide_advice || ($admin_access && empty($mail))) && (!empty($vote_requests) && is_array($vote_requests) && count($vote_requests) > 0)) : ?>
<h4>Advies:</h4>
<div class="ammo-section-content">
<p>
  <?php $options = ammo_motion_advice(); ?>
  <?php print ucfirst(strtolower($options[$advice])); ?>
  <?php if (!empty($advice_supplement)) : ?>
    <br/><?php print $advice_supplement; ?>
  <?php endif; ?>
</p>
</div>
<?php endif; ?>

<?php 
// Process vote requests - only show when there are actual vote requests
if (!empty($vote_requests) && is_array($vote_requests) && count($vote_requests) > 0) {
  print '<h4>Stemming aangevraagd door:</h4>';
  print '<div class="ammo-section-content">';
  $vote_list = array();
  foreach ($vote_requests as $vote_request) {
    $vote_list[] = $vote_request['contact_display_name'];
  }
  $last_vote = array_pop($vote_list);
  $votes_list = (count($vote_list) === 0) ? $last_vote : implode(', ', $vote_list) . ' en ' . $last_vote;
  print '<p>' . $votes_list . '.</p>';
  print '</div>';
}
?>
<?php if (empty($no_links)) : ?>
  <?php $dest = (!empty($destination) ? $destination : ammo_get_destination()); ?>
  <ul class="ammo-list">
    <?php if (!empty($owners_branch) || !empty($owners_member)) : ?>
      <?php if ($edit_access) : ?>
        <li>Bewerken:</li>
        <li><?php print l('inhoud', 'ammo/motion/edit/' . $entity_id, array('query' => $dest))?></li>
      <?php endif; ?>
      <?php if (($admin_access && $support_access) || $superadmin_access) : ?>
        <li><?php print l('status/advies', 'ammo/motion/advice/' . $entity_id, array('query' => $dest))?></li>
      <?php endif; ?>
      <?php if ($admin_access) : ?>
        <li><?php print l('indieners', 'ammo/motion/ownership/' . $entity_id, array('query' => $dest))?></li>
        <?php if ($state !== 'spelling_grammar') : ?>
          <li><?php print l('stemming', 'ammo/motion/vote-request/' . $entity_id, array('query' => $dest))?></li>
        <?php endif; ?>
      <?php endif; ?>
      <?php if ($superadmin_access) : ?>
        <li><?php print l('verwijderen', 'ammo/motion/delete/' . $entity_id, array('query' => $dest))?></li>
      <?php endif; ?>
    <?php endif; ?>
    <?php if (!empty($owners_branch) || !empty($owners_member)) : ?>
      <?php if ($withdraw_access && $state !== 'withdrawn' && $state !== 'spelling_grammar') : ?>
        <?php if ($unsupported_branches) : ?>
          <li><?php print l('mede indienen als afdeling', 'ammo/support/add/branch/motion/' . $entity_id, array('query' => $dest))?></li>
        <?php endif; ?>
        <?php if ($supported_branches && (empty($vote_requests) || !is_array($vote_requests) || count($vote_requests) == 0)) : ?>
          <li><?php print l('intrekken als afdeling', 'ammo/support/withdraw/branch/motion/' . $entity_id, array('query' => $dest))?></li>
        <?php endif; ?>
        <?php if ($removable_member_owners) : ?>
          <li><?php print l('intrekken individuele indieners', 'ammo/support/withdraw/branchmembers/motion/' . $entity_id, array('query' => $dest))?></li>
        <?php endif; ?>
      <?php endif; ?>
      <?php if ($vote_request_access) : ?>
        <?php if ($vote_unrequested_branches) : ?>
          <li><?php print l('stemming aanvragen als afdeling', 'ammo/vote_request/add/branch/motion/' . $entity_id, array('query' => $dest))?></li>
        <?php endif; ?>
        <?php if ($vote_requested_branches) : ?>
          <li><?php print l('stemming aanvraag intrekken als afdeling', 'ammo/vote_request/withdraw/branch/motion/' . $entity_id, array('query' => $dest))?></li>
        <?php endif; ?>
      <?php endif; ?>
    <?php endif; ?>
  </ul>
<?php endif; ?>

<?php if ($is_withdrawn): ?>
</div>
<?php endif; ?>
