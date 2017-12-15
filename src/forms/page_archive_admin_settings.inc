<?php

module_load_include('inc', 'page_archive', 'src/queries/scheduler.inc');

/**
 * Admin settings form for archive content.
 * @param $form
 * @param $form_state
 * @return mixed
 */
function page_archive_admin_settings($form, &$form_state) {
  global $base_url;

  $intro = '
  <h1>Archive Settings</h1>
  <hr>
  <h2>Adjust common settings for the Archive Module</h2>
  ';

  $form['archive']['settings']['intro'] = [
    '#id' => 'intro-settings',
    '#markup' => $intro,
  ];

  $form['archive']['settings']['archive_allowed_domains'] = [
    '#type' => 'textarea',
    '#title' => t('Allowed Domains List'),
    '#description' => 'Comma separated list of allowed domains.',
    '#theme' => 'textarea',
    '#default_value' => variable_get('archive_allowed_domains', $base_url),
  ];

  $form['archive']['settings']['archive_css_selectors'] = [
    '#type' => 'textarea',
    '#title' => t('Default CSS selectors'),
    '#description' => 'Comma separated list of default css selectors</br> #this-id , .this-class </br> Will first look for content in #this-id then will look for content in .this-class and concatenate all content together in order of selector.',
    '#theme' => 'textarea',
    '#default_value' => variable_get('archive_css_selectors', '.region-content'),
  ];

  $form['archive']['settings']['archive_batch_amount'] = [
    '#type' => "numberfield",
    '#title' => t('Batch Size'),
    '#required' => true,
    '#maxlength' => 4,
    '#theme' => 'numberfield',
    '#description' => t("Batch size determines the amount of archive pages that will be processed during the cron schedule."),
    '#default_value' => variable_get('archive_batch_amount', 5),
  ];

  $form['archive']['settings']['archive_delete_node'] = [
    '#title' => t('Delete node that was archived'),
    '#type' => 'checkboxes',
    '#description' => t('When selected, validation will check The node that was being archived will be deleted from the system.'),
    '#options' => [1 =>'Yes Delete Node'],
    '#default_value' => variable_get('archive_delete_node', 0)
  ];
  $link_to_cron = '<div style="margin-bottom: 2em">';
  $link_to_cron .= 'Follow link for <a href="/admin/config/system/cron">';
  $link_to_cron .= 'Cron Settings.</a></div>';

  $form['archive']['settings']['link_to_cron']['#markup'] = t($link_to_cron);


  return system_settings_form($form);
}
