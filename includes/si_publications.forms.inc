<?php
/**
 * @file
 * Provides the form functionality for the si_publications module
 */

/**
 * Implements hook_form
 * Creates the configuration form
 */
function si_publications_settings_form($form, &$form_state) {
  // get the list of fields for the chosen scientist bundle for later use
  $scientist_ct = variable_get('si_publications_scientist_ct');
  $si_profile_field = variable_get('si_publications_si_profile_field');
  $recent_papers_field = variable_get('si_publications_recent_papers_field');

  $form['si_publications_api_url_field'] = array(
    '#type' => 'textfield',
    '#title' => t('URL to the API on the Smithsonian network. (default: http://staff.research.si.edu/search/) '),
    '#required' => TRUE,
    '#size' => 60,
    '#default_value' => variable_get('si_publications_api_url_field', 'http://staff.research.si.edu/search/'),
  );

  $options = node_type_get_names();
  $options['users'] = 'User Accounts';

  $form['si_publications_scientist_ct'] = array(
    '#type' => 'select',
      '#title' => t('Your Scientist Content Type'),
      '#options' => $options,
      '#default_value' => $scientist_ct,
      '#description' => t('Select your scientist content type. It should contain a text field for "SI Profile ID" and a long text field for "Recent Papers".'),
      '#required' => TRUE,
      '#attributes' => array('onChange' => 'this.form.submit();'),
  );

  // get all fields, we will need this list twice.
  $field_list = null;
  if ($scientist_ct == 'users') {
    $field_list = field_info_instances('user', 'user');
  }
  else {
    $field_list = field_info_instances('node', $scientist_ct);
  }

  //get a list of text fields on the scientist content type so user can pick SI Profile ID field
  $text_fields = array();
  if (!empty($scientist_ct)) {
    foreach ($field_list as $field) {
      // we only want long text fields that have text processing enabled
      if ($field['widget']['type'] == 'text_textfield') {
        $text_fields[] = $field['field_name'];
        //must be associative array for later use as options value in Drupal FAPI
        $text_fields = drupal_map_assoc($text_fields);
      }
    }
  }

  //only show SI profiles field picker if there are options! Else show error message
  $form['si_publications_si_profile_field'] = array(
    '#type' => 'select',
    '#title' => t('Your SI profile ID field'),
    '#options' => !empty($text_fields) ? $text_fields : array(t('N/A')),
    '#default_value' => $si_profile_field,
    '#description' => !empty($text_fields) ? t('Select your SI Profiles ID field. <strong>It must be a text field.</strong>') : t('The content type "@ct" does not contain any text fields. Please add one.', array('@ct' => $scientist_ct)),
    '#disabled' => empty($text_fields) ? TRUE : FALSE,
    // '#required' => TRUE,
   );

  //get a list of long text fields on the scientist content type so user can pick Recent Papers field
  $longtext_fields = array();
  if (!empty($scientist_ct)) {
    foreach ($field_list as $field) {
      // we only want long text fields that have text processing enabled
      if ($field['widget']['type'] == 'text_textarea' && $field['settings']['text_processing'] == 1) {
        //check that cardinality is unlimited
        $field_info = field_info_field($field['field_name']);
        if ($field_info['cardinality'] == -1) {
          $longtext_fields[] = $field['field_name'];
        }
        //must be associative array for later use
        $longtext_fields = drupal_map_assoc($longtext_fields);
      }
    }
  }

  $form['si_publications_recent_papers_field'] = array(
    '#type' => 'select',
    '#title' => t('Your Recent Papers field'),
    '#options' => !empty($longtext_fields) ? $longtext_fields : array(t('N/A')),
    '#default_value' => $recent_papers_field,
    '#description' => !empty($longtext_fields) ? t('Select your Recent Papers field. <strong>It must be an unlimited value Long Text field. It also must allow HTML for formatting purposes.</strong>') : t('The content type "@ct" does not contain any long text, HTML-enabled, multivalue fields. Please add one. See the module readme for more information', array('@ct' => $scientist_ct)),
    // '#required' => TRUE,
    '#disabled' => empty($longtext_fields) ? TRUE : FALSE,
  );

  $form['si_publications_cron_switch'] = array(
    '#type' => 'checkbox',
    '#title' => t('Import via cron.'),
    '#default_value' => variable_get('si_publications_cron_switch'),
    '#description' => ($longtext_fields && $text_fields) ? t('Check this to have your publications imported every time cron runs. If you leave it unchecked, papers will only be imported when you <a href="/admin/content/publications">do so manually</a>.') : t('You cannot check this box until you have properly configured the content type and field settings above.'),
    //we cannot allow import via cron unless all fields are selected
    '#disabled' => ($longtext_fields && $text_fields) ? FALSE : TRUE,
  );

  $form['si_publications_papers_quantity'] = array(
    '#type' => 'textfield',
    '#title' => t('Number of papers to store for each scientist'),
    // '#required' => TRUE,
    '#size' => 4,
    '#default_value' => variable_get('si_publications_papers_quantity', '3') ,
    '#disabled' => ($longtext_fields && $text_fields) ? FALSE : TRUE,
  );

  $form['si_publications_altmetric_switch'] = array(
    '#type' => 'checkbox',
    '#title' => t('Include Altmetric icons next to citations'),
    '#default_value' => variable_get('si_publications_altmetric_switch'),
    '#disabled' => ($longtext_fields && $text_fields) ? FALSE : TRUE,
    '#description' => ($longtext_fields && $text_fields) ? t('<a href="https://www.altmetric.com/">Altmetric</a> is a 3rd party service that indicates how often a publication is used online. If you enable this feature, an icon will appear next to each publication that has a valid DOI.') : t('You cannot check this box until you have properly configured the content type and field settings above.'),

  );

  $form['submit'] =  array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}

/**
 * Implements hook_form_validate().
 *
 */
function si_publications_settings_form_validate($form, &$form_state) {
  //We need to ensure recent paper quantity is an integer between 1 and 10
  $paper_quantity = $form_state['values']['si_publications_papers_quantity'];
  if (!ctype_digit($paper_quantity) || ($paper_quantity > '10') || ($paper_quantity < '1')) {
    form_set_error('si_publications_papers_quantity', t('Field must be an integer greater zero but no more than 10'));
  }
}

/**
 * Implements hooks_form_submit().
 * code taken from system_settings_form_submit because it does important things, but we want to do our own important things too
 */
function si_publications_settings_form_submit($form, &$form_state) {

 //if scientist content type has changed, clear all other stored vars because they are no longer valid
  if ( $form_state['values']['si_publications_scientist_ct'] != variable_get('si_publications_scientist_ct') ) {
    $form_state['values']['si_publications_si_profile_field'] = '';
    $form_state['values']['si_publications_recent_papers_field'] = '';
    $form_state['values']['si_publications_cron_switch'] = 0;
    $form_state['values']['si_publications_altmetric_switch'] = 0;
  }

  // Exclude unnecessary elements.
  form_state_values_clean($form_state);

  //set variables
  foreach ($form_state['values'] as $key => $value) {
    if (is_array($value) && isset($form_state['values']['array_filter'])) {
      $value = array_keys(array_filter($value));
    }
    variable_set($key, $value);
  }

  drupal_set_message(t('Your SI Publications module settings have been saved.'));
}


function si_publications_import_form($form, &$form_state) {
  //check that both the si profiles field and recent papers field have been selected in settings
  if (variable_get('si_publications_si_profile_field') && variable_get('si_publications_recent_papers_field'))
  {
    $form['submit'] =  array(
      '#type' => 'submit',
      '#value' => t('Pull publications now'),
    );
    return $form;
  }
  else {
    drupal_set_message('You must complete the setttings form <a href="/admin/content/publications/settings">here</a> before you can manually pull publications.', 'error');
  }
}

/**
 * Implements hook_form_submit
 * Gathers scientist nodes and then fetches their latest publications.
 */
function si_publications_import_form_submit($form, &$form_state) {
  //get staff nodes
  module_load_include('inc', 'si_publications', 'includes/si_publications.controller');
  $scientist_nids = si_publications_load_scientists();

  // Decide if we are nodes or users and pull appropriately
  $nodes = null;
  $scientist_ct = variable_get('si_publications_scientist_ct');
  if ($scientist_ct == 'users') {
    $nodes = user_load_multiple($scientist_nids);
  }
  else {
    $nodes = node_load_multiple($scientist_nids);
  }

  //fetch publications for each scientist
  foreach ($nodes as $node) {
    $result = si_publications_fetch($node);
    drupal_set_message($result);
  }
}
