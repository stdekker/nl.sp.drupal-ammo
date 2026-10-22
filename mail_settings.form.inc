<?php

/**
 * Admin form with a main mail switch and, per step, a switch for each recipient.
 */
function ammo_mail_settings_form($form, &$form_state) {
  $form['ammo_mail_enabled'] = array(
    '#type' => 'checkbox',
    '#title' => 'E-mailmeldingen versturen',
    '#description' => 'Hoofdschakelaar voor alle e-mail van Ammo. Staat hij uit, dan gaat er geen enkele melding uit; de keuzes per stap hieronder blijven bewaard en gelden weer zodra hij aan staat.',
    '#default_value' => variable_get('ammo_mail_enabled', TRUE),
  );

  $form['steps'] = array(
    '#theme' => 'ammo_mail_settings_steps',
    '#tree' => TRUE,
  );
  $recipients = ammo_mail_recipients();
  $settings = ammo_mail_step_settings();
  foreach (ammo_mail_steps() as $step => $labels) {
    list($title, $description) = $labels;
    $form['steps'][$step]['#title'] = $title;
    $form['steps'][$step]['#description'] = $description;
    foreach ($recipients as $recipient => $recipient_label) {
      $form['steps'][$step][$recipient] = array(
        '#type' => 'checkbox',
        '#title' => $title . ': ' . $recipient_label,
        '#title_display' => 'invisible',
        '#default_value' => $settings[$step][$recipient],
        // Greyed out while the main switch is off; the submit handler then keeps the saved steps.
        '#states' => array(
          'disabled' => array(':input[name="ammo_mail_enabled"]' => array('checked' => FALSE)),
        ),
      );
    }
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Instellingen opslaan',
  );

  return $form;
}

/**
 * Saves the main switch, and the steps only while it is on, so switching off keeps them.
 */
function ammo_mail_settings_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  variable_set('ammo_mail_enabled', (bool) $values['ammo_mail_enabled']);
  // Browsers do not post disabled checkboxes, so with the switch off they would all read as off.
  if ($values['ammo_mail_enabled']) {
    $steps = array();
    foreach (ammo_mail_steps() as $step => $labels) {
      foreach (array_keys(ammo_mail_recipients()) as $recipient) {
        $steps[$step][$recipient] = !empty($values['steps'][$step][$recipient]);
      }
    }
    variable_set('ammo_mail_steps', $steps);
    drupal_set_message('De e-mailinstellingen zijn opgeslagen.');
  }
  else {
    drupal_set_message('Alle e-mailmeldingen staan uit. De keuzes per stap zijn bewaard.', 'warning');
  }
}

/**
 * Renders the steps of the mail settings form as a table with a checkbox column per recipient.
 */
function theme_ammo_mail_settings_steps($variables) {
  $element = $variables['element'];
  $header = array('Stap');
  foreach (ammo_mail_recipients() as $recipient_label) {
    $header[] = $recipient_label;
  }
  $rows = array();
  foreach (element_children($element) as $step) {
    $row = array('<strong>' . check_plain($element[$step]['#title']) . '</strong><div class="description">' . check_plain($element[$step]['#description']) . '</div>');
    foreach (array_keys(ammo_mail_recipients()) as $recipient) {
      $row[] = array('data' => drupal_render($element[$step][$recipient]), 'class' => array('recipient'));
    }
    $rows[] = $row;
  }
  return theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('class' => array('ammo-mail-settings')), 'sticky' => FALSE));
}
