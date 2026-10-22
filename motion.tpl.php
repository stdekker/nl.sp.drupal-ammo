<?php 
global $base_url; 
$is_withdrawn = ($state === 'withdrawn' || $rejected);
?>
<?php if ($show_heading): ?>
<h3 id="motion<?php print $entity_id; ?>" class="motion-header <?php print $is_withdrawn ? 'withdrawn-header' : ''; ?>">
  <?php print $heading; ?>
  <?php if ($state === 'withdrawn'): ?>
    <span class="withdrawn-indicator"> - Ingetrokken</span>
  <?php elseif ($rejected): ?>
    <span class="withdrawn-indicator"> - Afgewezen</span>
  <?php endif; ?>
</h3>
<?php endif; ?>

<?php if (!empty($properties)): ?>
<div class="ammo-properties">
  <?php foreach ($properties as $label => $value): ?>
    <h4><?php print $label; ?>:</h4>
    <div class="ammo-section-content"><p><?php print $value; ?></p></div>
  <?php endforeach; ?>
</div>
<?php endif; ?>

<?php if ($is_withdrawn): ?>
<a href="#" class="withdrawn-toggle" data-target="motion-content-<?php print $entity_id; ?>">[Toon details]</a>
<div id="motion-content-<?php print $entity_id; ?>" class="motion-content-collapsed" style="display: none;">
<?php endif; ?>
<?php //print '<p>' . $meeting_title . ' van de SP, in vergadering bijeen op ' . $meeting_date . '.</p>'; ?>

<?php if (!empty($first_owner)) : ?>
<h4>Hoofdindiener:</h4>
<div class="ammo-section-content">
  <p><?php print ammo_contact_names(array($first_owner)); ?></p>
  <?php if ($first_owner['contact_type'] === 'member') : ?>
    <p>Ondersteund door 50 leden (voldoende steun).</p>
  <?php endif; ?>
</div>
<?php if (!empty($co_owners)) : ?>
<h4>Mede-indieners:</h4>
<div class="ammo-section-content">
  <p><?php print ammo_contact_names($co_owners); ?></p>
</div>
<?php endif; ?>
<?php endif; ?>

<h4><?php print $consideration_opening; ?></h4>
<div class="ammo-section-content">
  <p class="text-limit"><?php print $consideration_body; ?></p>
</div>

<h4><?php print $follow_up_opening; ?></h4>
<div class="ammo-section-content">
  <p class="text-limit"><?php print $follow_up_body; ?></p>
</div>

<?php if (!empty($supplement)) : ?>
<h4>Toelichting voorstel:</h4>
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
</p>
</div>
<?php endif; ?>

<?php $show_advice = (!$hide_advice || ($admin_access && empty($mail))) && ammo_state_shows_advice($state); ?>
<?php if (!empty($advice) && $show_advice) : ?>
<h4>Advies:</h4>
<div class="ammo-section-content">
<p>
  <?php $options = ammo_motion_advice(); ?>
  <?php print ucfirst(strtolower($options[$advice])); ?>
</p>
</div>
<?php endif; ?>

<?php if (!empty($advice_supplement) && $show_advice) : ?>
<h4>Toelichting:</h4>
<div class="ammo-section-content">
  <p><?php print $advice_supplement; ?></p>
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
<?php if (empty($no_links) && !empty($actions)): ?>
<ul class="ammo-list">
  <li>Bewerken:</li>
  <?php foreach ($actions as $action): ?>
    <li><?php print l($action['title'], $action['href'], $action); ?></li>
  <?php endforeach; ?>
</ul>
<?php endif; ?>

<?php if ($is_withdrawn): ?>
</div>
<?php endif; ?>
