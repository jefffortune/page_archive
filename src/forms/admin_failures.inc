<?php

/**
 * @param $form
 * @param $form_state
 *
 * @return \Exception
 */
function page_archive_admin_view_failures($form, &$form_state) {
  try {
    $failed_query = db_select('page_archive_failures', 'l')
      ->extend('PagerDefault')
      ->fields('l', ['entity_id'])
      ->condition('error', 1)
      ->limit(25)
      ->execute()
      ->fetchAll();
  } catch (Exception $e) {
    return $e;
  }

  $headers = [
    t("Entity ID"),
    t("Title"),
    [
      'data' => t("Actions"),
      'colspan' => 2,
    ]
  ];

  $intro = '
  <div class="intro">
    <h1>Failed Archive Entities</h1>
    <hr>
    <h3>Manage your current failed entities.</h3>
  </div>
  ';

  $css = '
    <style>
        table {border-collapse: collapse;}
        tr td,
        tr th
        {width: 100px}
        tr td:nth-child(2), tr th:nth-child(2) {width: calc(100% - 300px);}
        tr th:nth-child(3) {
        width: 200px;
        }
    </style>';

  $form['archive']['view_failed']['intro'] = [
    '#markup' => t($intro),
  ];

  $form['archive']['view_failed']['style'] = [
    '#markup' => $css,
  ];

  $form['archive']['view_failed']['table'] = [
    '#prefix' => '<div id="files" style="margin: 4em">',
    '#suffix' => '</div>',
    '#tree' => TRUE,
    "#theme" => 'table',
    '#header' => $headers,
    '#rows' => []
  ];

  foreach ($failed_query as $item) {
    $entity_id = [
      '#id' => 'entity-' . $item->entity_id,
      '#markup' => $item->entity_id,
    ];

    $entity = entity_load('archive_entity', [$item->entity_id]);
    $title = [
      '#id' => 'entity-name--' . $entity[$item->entity_id]->id,
      '#markup' => $entity[$item->entity_id]->title,
    ];

    $failed_edit = [
      '#name' => $item->entity_id,
      '#type' => 'submit',
      '#value' => 'Edit Item',
      '#submit' => ['page_archive_edit_entity_submit'],
    ];

    $failed_remove = [
      '#name' => $item->entity_id,
      '#type' => 'submit',
      '#value' => 'Remove Item',
      '#submit' => ['page_archive_remove_entity_submit'],
    ];

    $form['archive']['view_failed']['table'][] = [
      'entity_id' => &$entity_id,
      'title' => &$title,
      'edit' => &$failed_edit,
      'remove' => &$failed_remove,
    ];


    $form['archive']['view_failed']['table']['#rows'][] = [
      ['data' => &$entity_id],
      ['data' => &$title],
      ['data' => &$failed_edit],
      ['data' => &$failed_remove]
    ];

    unset($title);
    unset($entity_id);
    unset($failed_remove);
    unset($failed_edit);
  }

  $form['archive']['view_failed']['pager'][] = [
    '#theme' => 'pager'
  ];

  return $form;
}


/**
 * Submit handler for the failures form to allow user to redirect to edit from of the selected entity.
 * @param $form
 * @param $form_state
 *
 */
function page_archive_edit_entity_submit($form, &$form_state) {
  global $base_url;
  $entity_id = $form_state['clicked_button']['#name'];
  drupal_goto("$base_url/admin/archive/archive_entity/manage/edit/$entity_id", ['query' => ['destination' => 'admin/archive/view-failures']]);
}

/**
 * Submit handler for removing a user selected entity. It will also remove from failures table.
 * @param $form
 * @param $form_state
 */
function page_archive_remove_entity_submit($form, &$form_state) {
  global $base_url;
  $entity_id = page_archive_sid_lookup_entity_id_schedule($form_state['clicked_button']['#name']);

  if ($entity_id === NULL) {
    $entity_id = $form_state['clicked_button']['#name'];
  }

  db_delete('page_archive_failures')
    ->condition('entity_id', $entity_id)
    ->execute();

  drupal_goto("$base_url/admin/archive/archive_entity/manage/delete/$entity_id", ['query' => ['destination' => 'admin/archive/view-failures']]);

}