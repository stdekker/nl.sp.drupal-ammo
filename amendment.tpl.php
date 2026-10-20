<?php
global $base_url;

$is_withdrawn = ($state === 'withdrawn');
$is_collapsed = $is_withdrawn || $state === 'invalid';
?>
<?php if ($show_heading): ?>
<h3 id="amendment<?php print $entity_id; ?>" class="amendment-header <?php print $is_collapsed ? 'collapsed-header' : ''; ?>">
  <?php print $heading; ?>
  <?php if ($is_collapsed): ?>
    <span class="collapsed-indicator"> - <?php print $is_withdrawn ? 'Ingetrokken' : 'Ongeldig'; ?></span>
  <?php endif; ?>
</h3>
<?php endif; ?>

<?php if ($is_collapsed): ?>
<a href="#" class="amendment-details-toggle" data-target="amendment-content-<?php print $entity_id; ?>">[Toon details]</a>
<div id="amendment-content-<?php print $entity_id; ?>" class="amendment-content-collapsed" style="display: none;">
<?php endif; ?>

<?php if (!empty($first_owner)): ?>
<div class="ammo-owners">
  <h4>Hoofdindiener:</h4>
  <p><?php print ammo_contact_names(array($first_owner)); ?></p>
  <?php if ($first_owner['contact_type'] === 'member'): ?>
    <p>Ondersteund door 50 leden (voldoende steun).</p>
  <?php endif; ?>
  <?php if (!empty($co_owners)): ?>
    <h4>Mede-indieners:</h4>
    <p><?php print ammo_contact_names($co_owners); ?></p>
  <?php endif; ?>
</div>
<?php endif; ?>

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
?>
<?php endif; ?>

<?php $show_advice = !$hide_advice || ($admin_access && empty($mail)); ?>
<?php if (!empty($advice) && $show_advice): ?>
<h4>Advies:</h4>
<p>
<?php
  $options = ammo_amendment_advice();
  print $options[$advice];
?>
</p>
<?php endif; ?>

<?php if (!empty($advice_supplement) && $show_advice): ?>
<h4>Toelichting:</h4>
<p><?php print $advice_supplement; ?></p>
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

<?php if (empty($no_links)): ?>
<?php $dest = (!empty($destination) ? $destination : ammo_get_destination()); ?>
<ul class="ammo-list">
  <?php if (!empty($owners_branch) || !empty($owners_member)): ?>
    <?php if ($edit_access): ?>
      <li>Bewerken:</li>
      <li><?php print l('voorstel wijzigen', 'ammo/amendment/edit/' . $entity_id, array('query' => $dest)); ?></li>
    <?php endif; ?>
    <?php if (($admin_access && $advice_access) || $superadmin_access): ?>
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
      <?php if ($unsupported_branches && ammo_can_co_submit($state)): ?>
        <li><?php print l('mede indienen', 'ammo/support/add/branch/amendment/' . $entity_id, array('query' => $dest)); ?></li>
      <?php endif; ?>
      <?php if ($supported_branches && !$withdraw_locked && (empty($vote_requests) || !is_array($vote_requests) || count($vote_requests) == 0)): ?>
        <li><?php print l('voorstel / steun intrekken', 'ammo/support/withdraw/branch/amendment/' . $entity_id, array('query' => $dest)); ?></li>
      <?php endif; ?>
      <?php if ($removable_member_owners && !$withdraw_locked): ?>
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

<?php if ($is_collapsed): ?>
</div>
<?php endif; ?>
