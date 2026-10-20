<?php

require_once __DIR__ . '/amendment-edit.inc';
require_once __DIR__ . '/locators.inc';

function ammo_entity_check_title($type, $title) {
  // check if title is unique
  $query = new EntityFieldQuery();
  $result = $query
    ->entityCondition('entity_type', $type)
    ->propertyCondition('title', $title)
    ->execute();
  if (!empty($result[$type])) {
    return FALSE;
  }
}

function ammo_error($message) {
  return '<section class="messages"><div class="messages error">'.$message.'</div></section>';
}

function ammo_states() {
  return array(
    'concept' => 'Concept',
    'submitted' => 'Ingediend',
    'accepted' => 'Overgenomen',
    //'accepted_intention' => 'Strekking overgenomen',
    //'accepted_partially' => 'Gedeeltelijk overgenomen',
    'rejected' => 'Afgewezen',
    'withdrawn' => 'Ingetrokken',
    'invalid' => 'Ongeldig',
    'blocked' => 'Verwijderd',
    'spelling_grammar' => 'Tekstuele wijziging',
  );
}

/**
 * Whether the current user may add support to an amendment in this state.
 */
function ammo_amendment_can_receive_support($state) {
  return ammo_admin_access() || !in_array($state, array('invalid', 'rejected'), TRUE);
}

function ammo_amendment_actions() {
  return array(
    'add' => 'Toevoegen tekst',
    'replace' => 'Vervangen tekst',
    'delete' => 'Schrappen tekst',
  );
}

function ammo_amendment_advice() {
  return array(
    'accept' => 'Overnemen',
    //'accept_amended' => 'Gewijzigd overnemen',
    //'accept_intention' => 'Strekking overnemen',
    //'accept_partially' => 'Gedeeltelijk overnemen',
    'reject' => 'Afwijzen',
  );
}

function ammo_motion_advice() {
  return array(
    'accept' => 'Overnemen',
    //'accept_amended' => 'Gewijzigd overnemen',
    //'accept_intention' => 'Strekking overnemen',
    //'accept_partially' => 'Gedeeltelijk overnemen',
    'reject' => 'Afwijzen',
  );
}

function ammo_get_afdelingen($contact_id) {
  $afdelingen = [];
  if (empty($contact_id)) {
    return [];
  }
  $afdelingen = &drupal_static(__FUNCTION__);
  if (!isset($afdelingen[$contact_id])) {
    $cache = cache_get('ammo_data');
    if (!empty($cache->data)) {
      $data = $cache->data;
      if (isset($data['afdelingen'][$contact_id])) {
        $afdelingen[$contact_id] = $data['afdelingen'][$contact_id];
        return $afdelingen[$contact_id];
      }
    }
    $afdelingen[$contact_id] = ammo_get_afdelingen_helper($contact_id);
    $data['afdelingen'][$contact_id] = $afdelingen[$contact_id];
    cache_set('ammo_data', $data, 'cache', time() + 86400);
  }
  return $afdelingen[$contact_id];
}

function ammo_get_afdelingen_helper($contact_id = NULL) {
  if (empty($contact_id)) {
    return FALSE;
  }

  if ($contact_id == 'all') {
    // Return all chapters.
    if ($afdelingen = ammo_get_all_afdelingen_list()) {
      return $afdelingen;
    }
  }
  elseif (ammo_add_access()) {
    // First try the complex procoid-based access system
    $afdeling_info = ammo_get_contact_add_access_afdelingen($contact_id);
    if (!empty($afdeling_info)) {
      return $afdeling_info;
    }
    
    // Fallback for users who have add access but don't have procoid function mappings
    // This ensures users with multiple afdelingen can still select which one to use
    $afdeling_info = ammo_get_contact_afdeling_info($contact_id);
    if (!empty($afdeling_info)) {
      return $afdeling_info;
    }
  }
  // No afdeling found.
  return FALSE;
}

function ammo_get_contact($contact_id) {
  $query = new EntityFieldQuery();
  $query
    ->entityCondition('entity_type', 'user')
    ->propertyCondition('status', '1', '=')
    ->fieldCondition('field_contact_id', 'value', $contact_id, '=');
  $result = $query->execute();

  if (isset($result['user'])) {
    $uid = array_key_first($result['user']);
    $user = user_load($uid);
    $user_wrapped = entity_metadata_wrapper('user', $user);
    return $user_wrapped;
  }
}

function ammo_get_contact_afdeling_info($contact_id) {
  $user_wrapped = ammo_get_contact($contact_id);
  if (!empty($user_wrapped)) {
    try {
      $afd_id = $user_wrapped->field_afdeling_id->value();
      $afd_title = $user_wrapped->field_afdeling->value();
      
      if (!empty($afd_id) && !empty($afd_title)) {
        return [$afd_id => $afd_title];
      }
    } catch (EntityMetadataWrapperException $e) {
      // Fields might not exist, return FALSE
    }
  }
  
  return FALSE;
}

function ammo_get_contact_add_access_afdelingen($contact_id) {
  global $user;
  $user_add_access_departments = [];

  // Get roles that give add access permission.
  $add_access_roles = user_roles(TRUE, 'add amendments and motions');
  // Get current user roles.
  $user_roles = $user->roles;
  // User roles that give access permission.
  $user_add_access_roles = array_intersect($user_roles, $add_access_roles);
  // Get function role mapping from procoid module.
  $function_role_mapping = variable_get('procoid_function_role_mapping', array());
  // Get all afdelingen.
  $all_departments = ammo_get_afdelingen('all');

  foreach ($user_add_access_roles as $add_access_role_id => $add_access_role) {
    // Check if role is mapped to function in procoid.
    if (!empty($function_role_mapping[$add_access_role_id])) {
      foreach ($function_role_mapping[$add_access_role_id] as $function_id) {
        if (!empty($function_id)) {
          // Check if user has function.
          $query = "SELECT did FROM {procoid_function} WHERE uid = :uid AND fid = :function_id";
          $results = db_query($query, array(":uid" => $user->uid, ':function_id' => $function_id));
          foreach ($results as $result) {
            if (!empty($all_departments[$result->did])) {
              $user_add_access_departments[$result->did] = $all_departments[$result->did];
            }
          }
        }
      }
    }
  }
  return $user_add_access_departments;
}

function ammo_get_all_afdelingen_list() {
  $query = new EntityFieldQuery();
  $query
    ->entityCondition('entity_type', 'node')
    ->entityCondition('bundle', 'afdeling')
    ->propertyCondition('status', '1', '=');
  
  $field_info = field_info_field('afd_status');
  if (!empty($field_info)) {
    $query->fieldCondition('afd_status', 'value', 'Erkend', '=');
  }

  $query->propertyOrderBy('title', 'ASC');
  $result = $query->execute();

  if (isset($result['node'])) {
    $nids = array_keys($result['node']);
    $items = node_load_multiple($nids);
    if (!empty($items)) {
      foreach ($items as $item) {
        $item_wrapped = entity_metadata_wrapper('node', $item);
        $afdelingen[$item_wrapped->afd_id->value()] = $item_wrapped->title->value();
      }
      return $afdelingen;
    }
  }
}

/*
 * Gets owners or backers for amendments or motions.
 *
 * @target_id: the amendment or motion id.
 * @target_type: either amendment or motion.
 * @relation_type: either backer or owner.
 * @contact_type: either member or branch.
 *
 */
function ammo_get_related_contacts($target_id, $target_type, $relation_type = NULL, $contact_type = NULL) {
  $contacts = array();
  // Get related contacts.
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'contact_relation')
    ->propertyCondition('target_id', $target_id)
    ->propertyCondition('target_type', $target_type);
  // Filter on relation_type if set.
  if (!empty($relation_type)) {
    $query->propertyCondition('relation_type', $relation_type);
  }
  // Filter on contact_type if set.
  if (!empty($contact_type)) {
    $query->propertyCondition('contact_type', $contact_type);
  }

  $query->propertyOrderBy('id', 'ASC');

  $result = $query->execute();

  if (isset($result['contact_relation'])) {
    $ids = array_keys($result['contact_relation']);
    $items = entity_load('contact_relation', $ids);
  }
  if (!empty($items)) {
    foreach ($items as $item) {
      $contacts[$item->contact_id] = array(
        'id' => $item->id,
        'contact_id' => $item->contact_id,
        'contact_email' => $item->contact_email,
        'contact_type' => $item->contact_type,
        'contact_display_name' => $item->contact_display_name,
        'branch_id' => $item->branch_id,
        'branch_display_name' => $item->branch_display_name,
      );
    }
  }
  return $contacts;
}

/*
 * Checks wether member or branch is related as owner or backer to amendment or motion.
 *
 * @target_id: the amendment or motion id.
 * @target_type: either amendment or motion.
 * @relation_type: either backer or owner.
 * @contact_type: either member or branch.
 */
function ammo_is_related($target_id, $target_type, $relation_type = NULL, $contact_type, $contact_id = NULL) {
  if ($contact_id === NULL) {
    global $user;
    $user_wrapper = entity_metadata_wrapper('user', $user);
    
    // Fallback for when OpenID is disabled - allow admins to test
    $contact_id = NULL;
    try {
      if (isset($user_wrapper->field_contact_id)) {
        $contact_id = $user_wrapper->field_contact_id->value();
      }
    } catch (EntityMetadataWrapperException $e) {
      // Field doesn't exist, use fallback
    }
    
    // If no contact_id and user is admin, use user ID as fallback
    if (empty($contact_id) && ammo_admin_access()) {
      $contact_id = $user->uid;
    }
  }
  // Get related contacts.
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'contact_relation')
    ->propertyCondition('target_id', $target_id)
    ->propertyCondition('target_type', $target_type)
    ->propertyCondition('contact_id', $contact_id)
    ->propertyCondition('contact_type', $contact_type);

  if (!empty($relation_type)) {
    $query->propertyCondition('relation_type', $relation_type);
  }

  $result = $query->execute();

  if (!empty($result)) {
    $tmp = array_pop($result);
    $tmp = array_pop($tmp);
    return $tmp->id;
  }
  return FALSE;
}

/*
 * @type: amendment or motion.
 * @branch_id: the id of the local branch.
 * @item_id: either meeting_id or document_id.
 */
function ammo_get_branch_amendments_or_motions($type, $branch_id, $item_id) {
  $data = array();
  $items = array();
  $property = ($type == 'amendment') ? 'document_id' : 'meeting_id';

  $query = db_select('ammo_' . $type . 's', 't');
  $query->join('ammo_contact_relations', 'r', 't.id = r.target_id');
  $query
    ->fields('t', array('id'))
    ->fields('r', array('id', 'branch_id'))
    //->condition('t.state', 'concept', '!=')
    ->condition('r.relation_type', 'owner','=')
    ->condition('r.contact_type', 'branch','=')
    ->condition('r.target_type', $type,'=')
    ->condition('t.' . $property, $item_id,'=');
  $result= $query->execute();
  if ($result) {
    while ($row = $result->fetchObject()) {
      $data_raw[$row->id][$row->r_id] = $row->branch_id;
    }
    foreach ($data_raw as $entity_id => $owners) {
      $first_owner = array_shift($owners);
      if ($first_owner == $branch_id) {
        $data[$entity_id] = $entity_id;
      }
    }
  }
  return $data;
}

function ammo_default($form_state, $field) {
  $default_value = NULL;
  if (isset($form_state['values'][$field]))
  {
    $default_value = $form_state['values'][$field];
  }
  elseif (isset($form_state['storage'][$field]))
  {
    $default_value = $form_state['storage'][$field];
  }
  return $default_value;
}

function ammo_save_entity($type, $values, $entity_id = NULL) {
  if (empty($entity_id)) {
    $entity = entity_create($type, array());
  }
  else {
    $entity = entity_load_single($type, $entity_id);
  }
  if (!empty($values)) {
    foreach ($values as $key => $value) {
      $entity->$key = $value;
    }
  }
  $result = entity_save($type, $entity);
  return ($result !== FALSE) ? $entity : FALSE;
}

function ammo_store($entity_type, $entity_id, $mapping, $form, &$form_state) {
  global $base_url;
  $states = ammo_states();
  $advices = $entity_type == 'amendment' ? ammo_amendment_advice() : ammo_motion_advice();
  $values = isset($form_state['values']) ? $form_state['values'] : array();
  $fields = [];
  $meeting_id = ammo_get_meeting_id($entity_type, $entity_id);
  $meeting = NULL;
  if (!empty($meeting_id) && is_numeric($meeting_id)) {
    $meeting = entity_load_single('meeting', $meeting_id);
  }
  $hide_advice = is_object($meeting) && isset($meeting->hide_advice) ? $meeting->hide_advice : FALSE;
  $hide_state = is_object($meeting) && isset($meeting->hide_state) ? $meeting->hide_state : FALSE;

  $original_entity = NULL;
  if (!empty($entity_id) && is_numeric($entity_id)) {
    $original_entity = entity_load_single($entity_type, $entity_id);
  }
  $differences = array();
  foreach ($mapping as $database_field => $form_field) {
    if (isset($values[$form_field])) {
      // Store values.
      $form_state['storage'][$form_field] = $values[$form_field];
    }
    if (!empty($original_entity)) {
      // Existing entity, check if there are changes.
      $storage_value = isset($form_state['storage'][$form_field]) ? $form_state['storage'][$form_field] : '';
      if ($original_entity->$database_field != $storage_value) {
        // Store all changed fields to save in database.
        $fields[$database_field] = $storage_value;

        // Skip differences for advice when hide advice is set.
        if (in_array($database_field, ['advice', 'advice_supplement'])) {
          if ($hide_advice) {
            continue;
          }
        }
        // Skip differences for state when hide state is set.
        if (in_array($database_field, ['state', 'state_supplement'])) {
          if ($hide_state) {
            continue;
          }
        }

        // Check for differences.
        if (!empty($values[$form_field])) {
          $diff_data = ammo_diff_fields($form_field);
          if (!empty($diff_data)) {
            $fieldname = $diff_data['name'];
            $differences[$database_field] = array(
              'fieldname' => $fieldname,
            );
            // Store diffline.
            switch ($diff_data['type']) {
              case 'text':
                $differences[$database_field]['diff'] = ammo_diffline($original_entity->$database_field, isset($values[$form_field]) ? $values[$form_field] : '');
                break;

              case 'value':
                switch ($form_field) {
                  case 'state':
                    $original = $original_entity->$database_field ? $states[$original_entity->$database_field] : '';
                    $new = isset($values[$form_field]) && isset($states[$values[$form_field]]) ? $states[$values[$form_field]] : '';
                    break;

                  case 'advice':
                    $original = $original_entity->$database_field ? $advices[$original_entity->$database_field] : '';
                    $new = isset($values[$form_field]) && isset($advices[$values[$form_field]]) ? $advices[$values[$form_field]] : '';
                    break;

                  default:
                    $original = $original_entity->$database_field ? $original_entity->$database_field : '';
                    $new = isset($values[$form_field]) ? $values[$form_field] : '';
                    break;
                }
                $differences[$database_field]['diff'] = $original . ' → ' . $new;
                break;
            }
          }
        }
      }
    }
    // If new entity.
    else {
      // Store all fields.
      $fields[$database_field] = isset($form_state['storage'][$form_field]) ? $form_state['storage'][$form_field] : '';
    }
  }

  $storage = $form_state['storage'];

  // Configured locators are stored together, so describe each change apart.
  // As for other fields, only changes submitted in this form step count.
  if ($entity_type == 'amendment' && !empty($original_entity) && isset($fields['locator_values'])) {
    $locators = ammo_document_locators($original_entity->document_id);
    if (array_filter(array_intersect_key($values, $locators), 'strlen')) {
      $differences += ammo_locator_differences($original_entity->document_id, $original_entity, $fields);
    }
  }

  // Store motion or amendment id and create short url.
  if (!empty($original_entity)) {
    switch ($entity_type) {
      case 'amendment':
        $group = ammo_amendment_group($original_entity);
        if (array_key_exists('group_value', $storage) || array_key_exists('chapter', $storage)) {
          $group = ammo_amendment_group(array(
            'group_value' => isset($storage['group_value']) ? $storage['group_value'] : NULL,
            'chapter' => isset($storage['chapter']) ? $storage['chapter'] : NULL,
          ));
        }
        if (empty($original_entity->chapterized_id) || $group !== ammo_amendment_group($original_entity)) {
          $fields['chapterized_id'] = ammo_next_chapterized_id($storage['document'], $group);
        }
        break;

      case 'motion':
        if (empty($original_entity->motion_id)) {
          $query = 'SELECT MAX(motion_id) FROM {ammo_motions} WHERE meeting_id = :meeting';
          $result = db_query($query, array(':meeting' => $storage['meeting']));
          $new_id = $result->fetchField() + 1;
          $fields['motion_id'] = $new_id;
        }
        break;
    }

    // Create short url.
    $long_url = $base_url . '/ammo/support/add/member/' . $entity_type . '/' . $entity_id;
    $code = ammo_shurly($entity_type, $entity_id);
    shurly_shorten($long_url, $code);
  }

  // Save entity.
  $entity = ammo_save_entity($entity_type, $fields, $entity_id);
  if (empty($entity)) {
    drupal_set_message('Waarden zijn niet opgeslagen.', 'error');
  }
  else {
    if (!empty($differences) || empty($original_entity)) {
      global $language;
      // Send mails if ammo values where changed.
      if (!empty($differences)) {
        $meeting_id = isset($form_state['storage']['meeting']) ? $form_state['storage']['meeting'] : NULL;
        if (!empty($meeting_id) && is_numeric($meeting_id)) {
          ammo_changes_update_mail($entity_type, $entity->id, $differences, $meeting_id);
        }
      }
    }
  }
  return $entity;
}

function ammo_populate_form($type, $entity_id, &$form_state) {
  $storage = &$form_state['storage'];
  $meeting_id = ammo_get_meeting_id($type, $entity_id);
  $meeting = entity_load_single('meeting', $meeting_id);

  // check if exists.
  $entity = entity_load_single($type, $entity_id);
  if ($entity) {
    // Check access.
    $first_owner = ammo_get_first_owner($type, $entity->id);
    $edit_access = empty($first_owner) ? FALSE : ammo_edit_access($meeting_id, $first_owner['branch_id'], $type, $entity_id);
    if ($edit_access) {
      $storage['branch_id'] = $first_owner['branch_id'];
      $storage['branch_display_name'] = $first_owner['branch_display_name'];
      $storage['meeting'] = $meeting_id;
      $storage['meeting_display_name'] = $meeting->title;
      $storage[$type] = $entity->id;

      // Get and store values from existing amendment/motion in form.
      $mapping = array(
        'amendment' => array(
          'document_id' => 'document',
          'chapter' => 'chapter',
          'page' => 'page',
          'line' => 'line',
          'supplement' => 'supplement',
          'advice' => 'advice',
          'advice_supplement' => 'advice_supplement',
          'state' => 'state',
          'state_supplement' => 'state_supplement',
          'chapterized_id' => 'chapterized_id',
          'amendment_text' => 'amendment_text',
          'locator_values' => 'locator_values',
          'group_value' => 'group_value',
        ),
        'motion' => array(
          'meeting_id' => 'meeting',
          'supplement' => 'supplement',
          'advice' => 'advice',
          'advice_supplement' => 'advice_supplement',
          'state' => 'state',
          'state_supplement' => 'state_supplement',
          'motion_id' => 'motion_id',
          'consideration_opening' => 'consideration_opening',
          'consideration_body' => 'consideration_text',
          'follow_up_opening' => 'follow_up_opening',
          'follow_up_body' => 'follow_up_text',
        ),
      );
      foreach ($mapping[$type] as $database_field => $form_field) {
        $storage[$form_field] = $entity->$database_field;
      }
      if ($type === 'amendment') {
        $document = entity_load_single('document', $form_state['storage']['document']);
        $storage['document_display_name'] = $document->title;
        // The form edits each locator in its own field.
        $storage += ammo_amendment_locator_values($entity, ammo_document_locators($document));
        if (!ammo_admin_access() && !empty($entity->state) && $entity->state !== 'concept') {
          $storage['submitted_edit'] = $entity->id;
          $storage['submitted_edit_signature'] = ammo_amendment_edit_signature($entity->id);
        }
      }
      return TRUE;
    }
    else {
      drupal_set_message('Data kon niet worden geladen.', 'error');
    }
  }
  else {
      drupal_set_message('Data kon niet worden geladen.', 'error');
  }
  return FALSE;
}

function ammo_afdelingen_access($admin_owner = NULL) {
  $afdelingen = [];

  // Afdelingen filteren op gebruikers id.
  if (ammo_admin_access() && !empty($admin_owner)) {
    $afdelingen = ammo_get_afdelingen('all');
    if (!is_array($afdelingen)) {
      $afdelingen = [];
    }
    $afdelingen = array(0 => '- ' . $admin_owner) + $afdelingen;
  }
  else {
    global $user;
    $user_wrapper = entity_metadata_wrapper('user', $user);
    
    // Fallback for when OpenID is disabled - allow admins to test
    $contact_id = NULL;
    try {
      if (isset($user_wrapper->field_contact_id)) {
        $contact_id = $user_wrapper->field_contact_id->value();
      }
    } catch (EntityMetadataWrapperException $e) {
      // Field doesn't exist, use fallback
    }
    
    // If no contact_id and user is admin, use user ID as fallback
    if (empty($contact_id) && ammo_admin_access()) {
      $contact_id = $user->uid;
    }
    
    $afdelingen = ammo_get_afdelingen($contact_id);
  }
  return $afdelingen;
}

function ammo_checked_form(&$form, &$form_state, $type) {
  // Separate instruction texts for easier modification.
  $instructions = array(
    'amendment' =>
      '<p>Afdelingsvoorzitters kunnen hier wijzigingsvoorstellen indienen. Dit zijn voorstellen tot het inhoudelijk wijzigen of aanvullen van het verkiezingsprogramma.</p>
      <p>Om de stemmingen op het congres vlot en goed te laten verlopen willen we je vragen om vooraf te controleren of het wijzigingsvoorstel dat jij in wilt dienen, al niet eerder door een andere afdeling ingediend is.</p>
      <p>In dat geval kun je het wijzigingsvoorstel met één druk op de knop &#39;mede-indienen&#39;. Mocht de eerste indiener het wijzigingsvoorstel intrekken, dan blijft het wijzigingsvoorstel staan omdat jij mede-indiener bent.</p>',
    'motion' =>
      '<p>Afdelingsvoorzitters kunnen hier moties indienen. Moties roepen de partij tot een specifieke actie.</p>
      <p>Om de stemmingen op het congres vlot en goed te laten verlopen willen we je vragen om vooraf te controleren of de motie die jij in wilt dienen, al niet eerder door een andere afdeling ingediend is. 
      In dat geval kun je de motie met één druk op de knop &#39;mede-indienen&#39;. Mocht de eerste indiener de motie intrekken, dan blijft de motie staan omdat jij mede-indiener bent.</p>',
  );

  $link1 = l('gecontroleerd', 'ammo/' . $type . 's', array('attributes' => array('target' => '_blank')));
  $text1 = ($type == 'amendment') ? 'vergelijkbaar wijzigingsvoorstel' : 'vergelijkbare motie';

  // Use the correct instruction text based on type, fallback to motion if unknown.
  $finaltext = isset($instructions[$type]) ? $instructions[$type] : $instructions['motion'];

  $form['checked'] = array(
    '#type' => 'checkbox',
    '#title' => 'Ik heb ' . $link1 . ' dat er niet al een ' . $text1 . ' is ingediend.',
    '#default_value' => FALSE,
    '#required' => TRUE,
    '#prefix' => $finaltext,
  );
}

function ammo_select_owner_form($type, &$form, &$form_state) {
  // Get meeting.
  $meeting = entity_load_single('meeting', $form_state['storage']['meeting']);
  // Backed by local branch?
  if (
    ammo_is_related($form_state['storage'][$type], $type, 'owner', 'branch', $form_state['storage']['branch_id']) ||
    !empty($form_state['storage']['new'])
  ) {
    $default = TRUE;
  }
  else {
    $default = FALSE;
  }
  $title = 'Ingediend door ' . check_plain($form_state['storage']['branch_display_name']);
  $form['backed'] = array(
    '#type' => 'checkbox',
    '#title' => $title,
    '#default_value' => $default,
    '#name' => 'backed',
    '#disabled' => $form_state['storage']['branch_id'],
  );
}

/*
 * @type: amendment or motion.
 * @entity_id: document or meeting.
 */
function ammo_select_ammo_form($type, &$form, &$form_state, $entity_id) {
  $title = ($type == 'amendment') ? 'Wijzigingsvoorstel' : 'Motie';
  $entities = ammo_get_branch_amendments_or_motions($type, $form_state['storage']['branch_id'], $entity_id);
  if (!empty($entities)) $title .= ' indienen of bewerken';
  $form[$type] = array(
    '#type' => 'select',
    '#title' => $title,
    '#options' => $entities,
    '#empty_option' => 'indienen',
    '#default_value' => ammo_default($form_state, $type),
    '#attributes' => array('onchange' => "jQuery('#action_button').mousedown()"),
  );
}

function ammo_select_afdeling_form(&$form, &$form_state, $type, $options) {
  $text1 = ($type == 'amendment') ? 'het wijzigingsvoorstel' : 'de motie';
  $form['branch_id'] = array(
    '#type' => 'select',
    '#title' => 'Indienen namens',
    '#description' => 'Kies namens welke afdeling u ' . $text1 . ' invoert.',
    '#options' => $options,
    '#default_value' => ammo_default($form_state, 'branch_id'),
    '#required' => true,
  );
}

function ammo_select_meeting_form(&$form, &$form_state, $options = array()) {
  if (empty($options)) {
    $options = ammo_get_meetings();
  }
  
  if (count($options) == 1) {
    // Only one meeting available, select it automatically
    $meeting_ids = array_keys($options);
    $selected_meeting_id = $meeting_ids[0];
    $selected_meeting_title = $options[$selected_meeting_id];
    
    $form['meeting'] = array(
      '#type' => 'value',
      '#value' => $selected_meeting_id,
    );
    $form['selected_meeting_display'] = array(
      '#type' => 'item',
      '#title' => 'Bijeenkomst',
      '#markup' => check_plain($selected_meeting_title),
    );
  } else {
    // Multiple meetings, show dropdown
    $form['meeting'] = array(
      '#type' => 'select',
      '#title' => 'Bijeenkomst',
      '#description' => 'Kies de bijeenkomst waar het wijzigingsvoorstel of de motie behandeld wordt.',
      '#options' => $options,
      '#default_value' => ammo_default($form_state, 'meeting'),
      '#required' => true,
    );
  }
}

/**
 * Confirmation page that soft-deletes an amendment or motion and mails the explanation.
 */
function ammo_delete_entity_form($type, $entity_id) {
  $entity = entity_load_single($type, $entity_id);
  if (!$entity) {
    drupal_set_message('Data kon niet worden opgehaald.', 'error');
    return array();
  }
  $label = $type === 'amendment' ? 'wijzigingsvoorstel ' . ammo_amendment_number($entity) : 'motie ' . ($entity->motion_id ?: $entity->id);
  $cancel = isset($_GET['destination']) && is_string($_GET['destination']) && !url_is_external($_GET['destination']) ? drupal_parse_url($_GET['destination']) : array('path' => ammo_item_view_path($type, $entity_id), 'query' => array(), 'fragment' => '');
  drupal_set_title(drupal_ucfirst($label) . ' verwijderen');
  if ($entity->state === 'blocked') {
    $form['deleted'] = array(
      '#markup' => '<p>Dit ' . ($type === 'amendment' ? 'wijzigingsvoorstel' : 'motie') . ' is al verwijderd.</p><p>' . l('Terug', $cancel['path'], array('query' => $cancel['query'], 'fragment' => $cancel['fragment'])) . '</p>',
    );
    return $form;
  }
  $form['question'] = array(
    '#markup' => '<p><strong>Weet je zeker dat je ' . check_plain($label) . ' wilt verwijderen?</strong> Het krijgt de status Verwijderd en is daarna alleen nog zichtbaar voor beheerders. Je kunt het herstellen door bij status/advies een andere status te kiezen.</p>',
  );
  $form['view'] = array(
    '#type' => 'item',
    '#markup' => theme($type, array('entity_id' => $entity_id, 'no_links' => TRUE)),
  );
  $form['mail_text'] = array(
    '#type' => 'textarea',
    '#title' => 'Uitleg',
    '#description' => 'Deze uitleg wordt gemaild naar de indieners.',
    '#required' => TRUE,
  );
  $form['entity_type'] = array(
    '#type' => 'value',
    '#value' => $type,
  );
  $form['entity_id'] = array(
    '#type' => 'value',
    '#value' => $entity->id,
  );
  $form['actions'] = array(
    '#type' => 'actions',
  );
  $form['actions']['delete'] = array(
    '#type' => 'submit',
    '#value' => 'Verwijderen',
    '#name' => 'delete_amot',
    '#id' => 'delete_amot',
    // Only the explanation needs validating, not the steps of the entry form.
    '#validate' => array(),
    '#submit' => array('ammo_delete_entity_form_submit'),
  );
  $form['actions']['cancel'] = array(
    '#markup' => l('Annuleren', $cancel['path'], array('query' => $cancel['query'], 'fragment' => $cancel['fragment'])),
  );
  return $form;
}

/**
 * The Verwijderen button of the status/advies step, which opens the confirmation page.
 */
function ammo_delete_button($type, $entity_id, $form_state) {
  $destination = isset($form_state['storage']['destination']) ? $form_state['storage']['destination'] : array();
  return array(
    '#type' => 'link',
    '#title' => 'Verwijderen',
    '#href' => 'ammo/' . $type . '/delete/' . $entity_id,
    '#options' => array('query' => $destination, 'attributes' => array('class' => array('button', 'ammo-delete-button'))),
  );
}

/**
 * Sets an amendment or motion to Verwijderd and mails the explanation.
 */
function ammo_delete_entity_form_submit($form, &$form_state) {
  $type = $form_state['values']['entity_type'];
  $entity = entity_load_single($type, $form_state['values']['entity_id']);
  $form_state['redirect'] = 'ammo';
  if (!$entity || !ammo_admin_access()) {
    drupal_set_message('Het was niet mogelijk om dit te verwijderen.', 'error');
    return;
  }
  $label = $type === 'amendment' ? 'Wijzigingsvoorstel ' . ammo_amendment_number($entity) : 'Motie ' . ($entity->motion_id ?: $entity->id);
  $entity->state = 'blocked';
  if (entity_save($type, $entity) === FALSE) {
    drupal_set_message('Het was niet mogelijk om ' . check_plain(drupal_strtolower($label)) . ' te verwijderen.', 'error');
    return;
  }
  ammo_delete_mail($type, $entity->id, ammo_get_meeting_id($type, $entity->id), $form_state['values']['mail_text']);
  drupal_set_message(check_plain($label) . ' is verwijderd.');
}

function ammo_support_access($meeting_id) {
  if (ammo_passed_date($meeting_id, 'event')) {
    return FALSE;
  }
  if (ammo_add_access()) {
    return TRUE;
  }
  return FALSE;
}

function ammo_withdraw_access($meeting_id) {
  if (ammo_passed_date($meeting_id, 'withdraw')) {
    return FALSE;
  }
  if (ammo_add_access()) {
    return TRUE;
  }
  return FALSE;
}

function ammo_vote_request_access($meeting_id, $status) {
  // Check if withdrawal date has passed
  if (ammo_passed_date($meeting_id, 'withdraw')) {
    // Allow admins to bypass date restrictions
    if (!ammo_admin_access()) {
      return FALSE;
    }
  }
  
  if (!ammo_add_access()) {
    return FALSE;
  }
  
  // Get allowed statuses from configuration, fall back to default if not set
  $allowed_statuses = variable_get('ammo_vote_request_allowed_statuses', array('accepted', 'rejected'));
  return in_array($status, $allowed_statuses);
}

function ammo_edit_access($meeting_id, $branch_id, $entity_type = NULL, $entity_id = NULL) {
  if (ammo_admin_access()) {
    return TRUE;
  }
  if ($entity_type === 'amendment' && !empty($entity_id)) {
    $entity = entity_load_single('amendment', $entity_id);
    if (!empty($entity->state) && $entity->state !== 'concept') {
      $meeting = entity_load_single('meeting', $meeting_id);
      $owner = ammo_get_first_owner('amendment', $entity_id);
      return !empty($meeting->allow_submitted_amendment_edit)
        && !in_array($entity->state, array('withdrawn', 'blocked'), TRUE)
        && ammo_get_meeting_id('amendment', $entity_id) == $meeting_id
        && ammo_add_access() && !empty($owner)
        && $owner['branch_id'] == $branch_id && ammo_access($owner['branch_id']);
    }
  }
  if (ammo_passed_date($meeting_id, 'submit')) {
    return FALSE;
  }
  
  // Check if proposal has been submitted (no longer a draft)
  if (!empty($entity_type) && !empty($entity_id)) {
    $entity = entity_load_single($entity_type, $entity_id);
    if (!empty($entity) && !empty($entity->state) && $entity->state !== 'concept') {
      // Only ammo admins can edit submitted proposals
      return FALSE;
    }
  }
  
  if (ammo_access($branch_id)) {
    return TRUE;
  }
  return FALSE;
}

/*
 * If branch id is set checks wether user owns branch and wether the
 * date has not yet passed. If branch is not set only the latter is
 * checked.
 *
 * @meeting_id: id of meeting.
 * @date_type: submit, withdraw or meeting date.
 */
function ammo_access($branch_id) {
  if (!empty($branch_id)) {
    global $user;
    $user_wrapper = entity_metadata_wrapper('user', $user);
    
    // Fallback for when OpenID is disabled - allow admins to test
    $contact_id = NULL;
    try {
      if (isset($user_wrapper->field_contact_id)) {
        $contact_id = $user_wrapper->field_contact_id->value();
      }
    } catch (EntityMetadataWrapperException $e) {
      // Field doesn't exist, use fallback
    }
    
    // If no contact_id and user is admin, use user ID as fallback
    if (empty($contact_id) && ammo_admin_access()) {
      $contact_id = $user->uid;
    }
    
    $afdelingen = ammo_get_afdelingen($contact_id);
    if (is_array($afdelingen)) {
      if (array_key_exists($branch_id, $afdelingen)) {
        return TRUE;
      }
    }
  }
  return FALSE;
}

function ammo_ajax_redirect($destination = NULL) {
  ctools_include('ajax');
  ctools_add_js('ajax-responder');
  $commands[] = ctools_ajax_command_redirect($destination);
  print ajax_render($commands);
  exit;
}

function ammo_set_destination(&$form_state) {
  $destination = drupal_get_destination();
  if (!isset($form_state['storage']['destination'])) {
    $form_state['storage']['destination'] = $destination;
  }
}

function ammo_get_destination() {
  $destination = drupal_get_destination();
  if ($destination['destination'] !== 'system/ajax') {
    return $destination;
  }
}

function ammo_save_relation($relation_type, $target_type, $target_id, $contact_id, $contact_type, $contact_display_name, $branch_id = NULL, $branch_display_name = NULL) {
  $contact_display_name = ammo_cleanup_branch_name($contact_display_name);
  $branch_display_name = ammo_cleanup_branch_name($branch_display_name);
  // Get contact email.
  if ($contact_id == 0) {
    $contact_email = $meeting->email;
  }
  else {
    switch ($contact_type) {
      case 'branch':
        $contact_email = ammo_get_branch_email($contact_id);
        break;
      case 'member':
        if ($contact_id == 0) {
          $meeting_id = ammo_get_meeting_id($target_type, $target_id);
          $contact_email = ammo_mailadress($meeting_id);
        }
        else {
          $contact_email = ammo_get_contact_email($contact_id);
        }
        break;
    }
  }
  $mapping = array(
    'target_type' => $target_type,
    'target_id' => $target_id,
    'relation_type' => $relation_type,
    'contact_id' => $contact_id,
    'contact_email' => $contact_email,
    'contact_type' => $contact_type,
    'contact_display_name' => $contact_display_name,
    'branch_id' => $branch_id,
    'branch_display_name' => $branch_display_name,
  );
  $entity = ammo_save_entity('contact_relation', $mapping);
  if (empty($entity)) {
    drupal_set_message('Relatie is niet opgeslagen.' , 'error');
  }
  return $entity;
}

function ammo_diff($from, $to)
{
  $diffValues = array();
  $diffMask = array();

  $dm = array();
  $n1 = count($from);
  $n2 = count($to);

  for ($j = -1; $j < $n2; $j++) $dm[-1][$j] = 0;
  for ($i = -1; $i < $n1; $i++) $dm[$i][-1] = 0;
  for ($i = 0; $i < $n1; $i++)
  {
    for ($j = 0; $j < $n2; $j++)
    {
      if ($from[$i] == $to[$j])
      {
        $ad = $dm[$i - 1][$j - 1];
        $dm[$i][$j] = $ad + 1;
      }
      else
      {
        $a1 = $dm[$i - 1][$j];
        $a2 = $dm[$i][$j - 1];
        $dm[$i][$j] = max($a1, $a2);
      }
    }
  }

  $i = $n1 - 1;
  $j = $n2 - 1;
  while (($i > -1) || ($j > -1))
  {
    if ($j > -1)
    {
      if ($dm[$i][$j - 1] == $dm[$i][$j])
      {
        $diffValues[] = $to[$j];
        $diffMask[] = 1;
        $j--;
        continue;
      }
    }
    if ($i > -1)
    {
      if ($dm[$i - 1][$j] == $dm[$i][$j])
      {
        $diffValues[] = $from[$i];
        $diffMask[] = -1;
        $i--;
        continue;
      }
    }
      {
        $diffValues[] = $from[$i];
        $diffMask[] = 0;
        $i--;
        $j--;
      }
  }

  $diffValues = array_reverse($diffValues);
  $diffMask = array_reverse($diffMask);

  return array('values' => $diffValues, 'mask' => $diffMask);
}

function ammo_diffline($line1, $line2)
{
  $diff = ammo_diff(str_split($line1), str_split($line2));
  $diffval = $diff['values'];
  $diffmask = $diff['mask'];

  $n = count($diffval);
  $pmc = 0;
  $result = '';
  for ($i = 0; $i < $n; $i++)
  {
    $mc = $diffmask[$i];
    if ($mc != $pmc)
    {
      switch ($pmc)
      {
      case -1: $result .= '</del>'; break;
      case 1: $result .= '</ins>'; break;
      }
      switch ($mc)
      {
      case -1: $result .= '<del>'; break;
      case 1: $result .= '<ins>'; break;
      }
    }
    $result .= $diffval[$i];

    $pmc = $mc;
  }
  switch ($pmc)
  {
  case -1: $result .= '</del>'; break;
  case 1: $result .= '</ins>'; break;
  }

  return $result;
}

function ammo_mail_get_params($entity_type, $entity_id, $meeting_id, $sender_email, $owner = NULL) {
  if (empty($entity_id) || !is_numeric($entity_id) || empty($meeting_id) || !is_numeric($meeting_id)) {
    return array();
  }
  
  $entity = entity_load_single($entity_type, $entity_id);
  $meeting = entity_load_single('meeting', $meeting_id);
  
  if (empty($entity) || empty($meeting)) {
    return array();
  }
  
  $title = $meeting->title;
  // Get first owner.
  if (ammo_admin_access() && !empty($meeting->admin_owner)) {
    $first_owner = $meeting->admin_owner;
  }
  else {
    $first_owner_data = ammo_get_first_owner($entity_type, $entity_id);
    $first_owner = $first_owner_data['contact_display_name'];
  }
  switch ($entity_type) {
    case 'amendment':
      if (!empty($entity->document_id) && is_numeric($entity->document_id)) {
        $document = entity_load_single('document', $entity->document_id);
        if (!empty($document)) {
          $title = $title . ': ' . $document->title;
        }
      }
      $id = ammo_amendment_number($entity);
      break;

    case 'motion':
      $id = $entity->motion_id;
      break;
  }
  // Get recipients.
  $rest_owners = ammo_get_related_contacts($entity_id, $entity_type, 'owner', 'branch');
  $rest = (empty($rest_owners)) ? FALSE : TRUE;
  // Set params.
  $params = array(
    'reply_to' => $sender_email,
    'entity_type' => $entity_type,
    'entity_id' => $entity_id,
    'id' => $id,
    'markup' => '<h3>' . $title . '</h3>' . theme($entity_type, array('entity_id' => $entity_id, 'no_links' => TRUE, 'mail' => TRUE)),
    'meeting_id' => $meeting_id,
    'rest' => $rest,
    'first_owner' => $first_owner,
  );
  if ($entity_type == 'amendment') {
    $params['chapter'] = ammo_amendment_group($entity);
    $params['chapterized_id'] = $entity->chapterized_id;
    if (!empty($entity->document_id) && is_numeric($entity->document_id)) {
      $document = entity_load_single('document', $entity->document_id);
      if (!empty($document)) {
        $params['document_title'] = $document->title;
      }
    }
  }
  if (!empty($owner)) {
    $params['branch_display_name'] = $owner->branch_display_name;
  }
  return $params;
}

function ammo_changes_update_mail($entity_type, $entity_id, $differences, $meeting_id) {
  $meeting = entity_load_single('meeting', $meeting_id);
  global $language;
  $sender_email = ammo_mailadress($meeting_id);
  $relations = ammo_get_related_contacts($entity_id, $entity_type, 'owner', 'branch');
  $params = ammo_mail_get_params($entity_type, $entity_id, $meeting_id, $sender_email);
  $params['differences'] = $differences;
  $result = drupal_mail('ammo', 'change_message', $sender_email, $language, $params, $sender_email, TRUE);
  $success = !empty($result['result']);
  if (!empty($differences)) {
    foreach ($relations as $relation) {
      if (!empty($relation['contact_email'])) {
        $result = drupal_mail('ammo', 'change_message', $relation['contact_email'], $language, $params, $sender_email, TRUE);
        $success = !empty($result['result']) && $success;
      }
    }
  }
  return $success;
}

function ammo_withdraw_mail($entity_type, $entity_id, $owner, $meeting_id) {
  global $language;
  $sender_email = ammo_mailadress($meeting_id);
  $relations = ammo_get_related_contacts($entity_id, $entity_type);
  $params = ammo_mail_get_params($entity_type, $entity_id, $meeting_id, $sender_email, $owner);
  drupal_mail('ammo', 'withdraw_ownership', $sender_email, $language, $params, $sender_email, TRUE);
  if (!empty($owner->contact_email)) {
    drupal_mail('ammo', 'withdraw_ownership', $owner->contact_email, $language, $params, $sender_email, TRUE);
  }
  foreach ($relations as $relation) {
    if (!empty($relation['contact_email'])) {
      drupal_mail('ammo', 'withdraw_ownership', $relation['contact_email'], $language, $params, $sender_email, TRUE);
    }
  }
}

function ammo_support_mail($entity_type, $entity_id, $owner, $meeting_id) {
  global $language;
  $sender_email = ammo_mailadress($meeting_id);
  $relations = ammo_get_related_contacts($entity_id, $entity_type);
  $params = ammo_mail_get_params($entity_type, $entity_id, $meeting_id, $sender_email, $owner);
  drupal_mail('ammo', 'support', $sender_email, $language, $params, $sender_email, TRUE);
  foreach ($relations as $relation) {
    if (!empty($relation['contact_email'])) {
      drupal_mail('ammo', 'support', $relation['contact_email'], $language, $params, $sender_email, TRUE);
    }
  }
}

function ammo_delete_mail($entity_type, $entity_id, $meeting_id, $mail_text) {
  global $language;
  $sender_email = ammo_mailadress($meeting_id);
  $relations = ammo_get_related_contacts($entity_id, $entity_type);
  $params = ammo_mail_get_params($entity_type, $entity_id, $meeting_id, $sender_email);
  $params['mail_text'] = '<p>' . check_plain($mail_text) . '</p>';
  drupal_mail('ammo', 'delete_message', $sender_email, $language, $params, $sender_email, TRUE);
  foreach ($relations as $relation) {
    if (!empty($relation['contact_email'])) {
      drupal_mail('ammo', 'delete_message', $relation['contact_email'], $language, $params, $sender_email, TRUE);
    }
  }
}

function ammo_vote_request_mail($entity_type, $entity_id, $vote_request, $meeting_id) {
  global $language;
  $sender_email = ammo_mailadress($meeting_id);
  $relations = ammo_get_related_contacts($entity_id, $entity_type);
  $params = ammo_mail_get_params($entity_type, $entity_id, $meeting_id, $sender_email, $vote_request);
  drupal_mail('ammo', 'vote_request', $sender_email, $language, $params, $sender_email, TRUE);
  foreach ($relations as $relation) {
    if (!empty($relation['contact_email'])) {
      drupal_mail('ammo', 'vote_request', $relation['contact_email'], $language, $params, $sender_email, TRUE);
    }
  }
}

function ammo_vote_request_withdraw_mail($entity_type, $entity_id, $vote_request, $meeting_id) {
  global $language;
  $sender_email = ammo_mailadress($meeting_id);
  $relations = ammo_get_related_contacts($entity_id, $entity_type);
  $params = ammo_mail_get_params($entity_type, $entity_id, $meeting_id, $sender_email, $vote_request);
  drupal_mail('ammo', 'vote_request_withdraw', $sender_email, $language, $params, $sender_email, TRUE);
  if (!empty($vote_request->contact_email)) {
    drupal_mail('ammo', 'vote_request_withdraw', $vote_request->contact_email, $language, $params, $sender_email, TRUE);
  }
  foreach ($relations as $relation) {
    if (!empty($relation['contact_email'])) {
      drupal_mail('ammo', 'vote_request_withdraw', $relation['contact_email'], $language, $params, $sender_email, TRUE);
    }
  }
}

function ammo_get_contact_email($contact_id) {
  $user_wrapped = ammo_get_contact($contact_id);
  if ($email = $user_wrapped->field_email_primary->value()) {
    return $email;
  }
}

function ammo_get_branch_email($contact_id) {
  $branch_wrapped = ammo_get_branch($contact_id);
  $test = $branch_wrapped->afd_email->value();
  if ($email = $branch_wrapped->afd_email->value()) {
    return $email;
  }
}

function ammo_get_branch($contact_id) {
  $query = new EntityFieldQuery();
  $query
    ->entityCondition('entity_type', 'node')
    ->entityCondition('bundle', 'afdeling')
    ->propertyCondition('status', '1', '=');

  $field_info = field_info_field('afd_status');
  if (!empty($field_info)) {
    $query->fieldCondition('afd_status', 'value', 'Erkend', '=');
  }
  
  $query->fieldCondition('afd_id', 'value', $contact_id, '=');
  $result = $query->execute();

  if (isset($result['node'])) {
    $nid = array_key_first($result['node']);
    $chapter = node_load($nid);
    $chapter_wrapped = entity_metadata_wrapper('node', $chapter);
    return $chapter_wrapped;
  }
}

function ammo_diff_fields($source) {
  $translation = array(
    'chapter' => array(
      'name' => 'Hoofdstuk',
      'type' => 'value',
    ),
    'page' => array(
      'name' => 'Pagina',
      'type' => 'value',
    ),
    'line' => array(
      'name' => 'Regel',
      'type' => 'value',
    ),
    'consideration_opening' => array(
      'name' => 'Openingsregel overweging',
      'type' => 'text',
    ),
    'consideration_text' => array(
      'name' => 'Inhoud overweging',
      'type' => 'text',
    ),
    'follow_up_opening' => array(
      'name' => 'Openingsregel vervolg',
      'type' => 'text',
    ),
    'follow_up_text' => array(
      'name' => 'Inhoud vervolg',
      'type' => 'text',
    ),
    'supplement' => array(
      'name' => 'Toelichting',
      'type' => 'text',
    ),
    'amendment_text' => array(
      'name' => 'Wijzigingsvoorstel tekst',
      'type' => 'text',
    ),
    'advice' => array(
      'name' => 'Advies',
      'type' => 'value',
    ),
    'advice_supplement' => array(
      'name' => 'Toelichting op advies',
      'type' => 'text',
    ),
    'state' => array(
      'name' => 'Status',
      'type' => 'value',
    ),
    'state_supplement' => array(
      'name' => 'Toelichting op statuswijziging',
      'type' => 'text',
    ),
  );
  return (isset($translation[$source])) ? $translation[$source] : FALSE;
}

function ammo_get_first_owner($entity_type, $entity_id) {
  $owners = ammo_get_related_contacts($entity_id, $entity_type, 'owner', 'member');
  if (empty($owners)) {
    $owners = ammo_get_related_contacts($entity_id, $entity_type, 'owner', 'branch');
  }
  $first_owner = array_shift($owners);
  if (!empty($first_owner)) {
    return $first_owner;
  }
  return FALSE;
}

function ammo_passed_date($meeting_id, $date_type) {
  switch ($date_type) {
  case 'event':
    $field = 'date';
    break;
  case 'submit':
    $field = 'submission_closing_date';
    break;
  case 'withdraw':
    $field = 'withdrawal_closing_date';
    break;
  }
  $meeting = entity_load_single('meeting', $meeting_id);
  if (!empty($meeting->$field)) {
    $now = new DateTime();
    $last_date = new DateTime($meeting->$field);
    if ($now < $last_date) {
      return FALSE;
    }
  }
  return TRUE;
}

/*
 * @entity_type: amendment or motion.
 * @entity_id: amendment or motion id.
 */
function ammo_get_meeting_id($entity_type, $entity_id) {
  // Get meeting_id to check dates.
  if (empty($entity_id) || !is_numeric($entity_id)) {
    return FALSE;
  }
  
  $entity = entity_load_single($entity_type, $entity_id);
  if (empty($entity)) {
    return FALSE;
  }
  
  if ($entity_type == 'amendment' && !empty($entity->document_id) && is_numeric($entity->document_id)) {
    $entity = entity_load_single('document', $entity->document_id);
    if (empty($entity)) {
      return FALSE;
    }
  }
  
  if (!empty($entity->meeting_id)) {
    return $entity->meeting_id;
  }
  return FALSE;
}

function ammo_mailadress($meeting_id) {
  $meeting = entity_load_single('meeting', $meeting_id);
  $sender_mail = (!empty($meeting->email)) ? $meeting->email : variable_get('site_mail', 'webmaster@sp.nl');
  return $sender_mail;
}

function ammo_get_meetings($filter_type = NULL) {
  // Check if there is more than one option to choose from.
  $meetings = entity_load('meeting', FALSE, array(), TRUE);
  $options = array();
  foreach ($meetings as $meeting) {
    if (empty($meeting->hide_meeting)) {
      if (empty($filter_type)) {
        $options[$meeting->id] = $meeting->title;
      }
      else {
        if (!ammo_passed_date($meeting->id, $filter_type)) {
          $options[$meeting->id] = $meeting->title;
        }
      }
    }
  }
  return $options;
}

function ammo_get_documents($form_state = array()) {
  $documents = entity_load('document');
  $options = array();
  foreach ($documents as $document) {
    if (empty($form_state) || $document->meeting_id == $form_state['storage']['meeting']) {
      $options[$document->id] = $document->title;
    }
  }
  return $options;
}

/*
 * Checks wether there are member owners of a motion or amendment that may
 * be removed by the branch chairman.
 *
 */
function ammo_removable_member_owners($target_type, $entity_id) {
  global $user;
  $user_wrapper = entity_metadata_wrapper('user', $user);
  
  // Fallback for when OpenID is disabled - allow admins to test
  $contact_id = NULL;
  try {
    if (isset($user_wrapper->field_contact_id)) {
      $contact_id = $user_wrapper->field_contact_id->value();
    }
  } catch (EntityMetadataWrapperException $e) {
    // Field doesn't exist, use fallback
  }
  
  // If no contact_id and user is admin, use user ID as fallback
  if (empty($contact_id) && ammo_admin_access()) {
    $contact_id = $user->uid;
  }

  $contacts = ammo_get_related_contacts($entity_id, $target_type, 'owner', 'member');
  // Controleer of de betreffende leden lid zijn van afdeling voorzitter.
  $afdelingen = ammo_get_afdelingen($contact_id);
  if (!empty($afdelingen) && !empty($contacts)) {
    $afdelingen_ids = array_keys($afdelingen);
    foreach ($contacts as $member) {
      if (!in_array($member['branch_id'], $afdelingen_ids) && !ammo_superadmin_access()) {
        return FALSE;
      }
    }
  }
  else {
    return FALSE;
  }
  return TRUE;
}

function ammo_shurly($type, $id) {
  $prefix = ($type === 'amendment') ? 'a' : 'm';
  $code = $prefix . base_convert($id, 10, 36);
  return $code;
}

/**
* Removes invalid XML
*
* @access public
* @param string $value
* @return string
*/
function ammo_strip_invalid_xml($value) {
  $ret = "";
  $current;
  if (empty($value))
  {
    return $ret;
  }

  $length = strlen($value);
  for ($i=0; $i < $length; $i++)
  {
    $current = ord($value[$i]);
    if (($current == 0x9) ||
    ($current == 0xA) ||
    ($current == 0xD) ||
    (($current >= 0x20) && ($current <= 0xD7FF)) ||
    (($current >= 0xE000) && ($current <= 0xFFFD)) ||
    (($current >= 0x10000) && ($current <= 0x10FFFF)))
    {
      $ret .= chr($current);
    }
    else
    {
      $ret .= " ";
    }
  }
  return $ret;
}

function ammo_cleanup_branch_name($branch_name) {
  if (strpos($branch_name, 'SP ') === 0) {
    $branch_name = substr($branch_name, 3);
  }
  return $branch_name;
}

function ammo_get_vote_requests($entity_type, $entity_id) {
  return ammo_get_related_contacts($entity_id, $entity_type, 'vote_request', 'branch');
}

/**
 * A step heading of a multistep form, left out when the page title already says it.
 */
function ammo_step_heading($heading) {
  return check_plain($heading) === drupal_get_title() ? '' : '<h2>' . check_plain($heading) . '</h2>';
}

/**
 * States an admin can choose; Verwijderd only for items that are already deleted.
 */
function ammo_editable_states($current_state = NULL) {
  $states = ammo_states();
  if ($current_state !== 'blocked') {
    unset($states['blocked']);
  }
  return $states;
}

/**
 * Validates legacy chapter, page and line values against a document.
 *
 * @param int $document_id
 *   The document ID to check ranges against.
 * @param int $chapter
 *   The chapter number to validate.
 * @param int $page
 *   The page number to validate.
 * @param int $line
 *   The line number to validate.
 *
 * @return array
 *   Array of validation errors. Empty array if all values are valid.
 */
function ammo_validate_document_ranges($document_id, $chapter, $page, $line) {
  if (empty($document_id) || !is_numeric($document_id)) {
    return array();
  }
  $document = entity_load_single('document', $document_id);
  if (!$document) {
    return array();
  }
  return ammo_validate_locators($document, array('chapter' => $chapter, 'page' => $page, 'line' => $line), ammo_legacy_locators($document));
}

/**
 * Gets document validation constraints as a human-readable string.
 *
 * @param int $document_id
 *   The document ID to get constraints for.
 *
 * @return string
 *   Formatted constraints string, or empty string if no constraints.
 */
function ammo_get_document_constraints_display($document_id) {
  if (empty($document_id) || !is_numeric($document_id)) {
    return '';
  }
  $document = entity_load_single('document', $document_id);
  if (!$document) {
    return '';
  }
  $constraints = array();
  foreach (ammo_document_locators($document) as $locator) {
    if ($locator['type'] === 'text' || ($locator['min'] === '' && $locator['max'] === '')) {
      continue;
    }
    if ($locator['min'] !== '' && $locator['max'] !== '') {
      $range = $locator['min'] . '-' . $locator['max'];
    }
    elseif ($locator['min'] !== '') {
      $range = 'vanaf ' . $locator['min'];
    }
    else {
      $range = 'tot ' . $locator['max'];
    }
    $constraints[] = $locator['label'] . ': ' . $range;
  }
  return implode(' | ', $constraints);
}
