<?php
module_load_include('inc', 'page_archive', 'src/queries/schedule');

/**
 * This form will have two options of edit or delete schedule entities waiting to be archived.
 *
 * @param $form
 * @param $form_state
 * @return mixed
 */
function page_archive_view_schedule($form, $form_state) {
  try {
    $schedule_query = db_select('page_archive_scheduled', 'c')
      ->extend('PagerDefault')
      ->fields('c', ['schedule_id', 'entity_id'])
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
    <h1>Scheduled Views</h1>
    <hr>
    <h3>Manage the current listing of pages in the archive queue.</h3>
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

  $form['archive']['view_schedule']['intro'] = [
    '#markup' => t($intro),
  ];

  $form['archive']['view_schedule']['style'] = [
    '#markup' => $css,
  ];

  $form['archive']['view_schedule']['table'] = [
    '#prefix' => '<div id="files" style="margin: 4em">',
    '#suffix' => '</div>',
    '#tree' => TRUE,
    "#theme" => 'table',
    '#header' => $headers,
    '#rows' => []
  ];

  foreach ($schedule_query as $item) {

    $schedule_id = [
      '#id' => 'archive-' . $item->schedule_id,
      '#markup' => $item->schedule_id,
    ];

    $entity_id = [
      '#id' => 'entity-' . $item->entity_id,
      '#markup' => $item->entity_id,
    ];

    $entity = entity_load_single('archive_entity', $item->entity_id);

    $title = [
      '#id' => 'entity-name--' . $entity->id,
      '#markup' => $entity->title,
    ];

    $schedule_edit = [
      '#name' => $item->entity_id,
      '#type' => 'submit',
      '#value' => 'Edit Item',
      '#submit' => ['page_archive_edit_entity_submit'],
    ];

    $schedule_remove = [
      '#name' => $item->schedule_id,
      '#type' => 'submit',
      '#value' => 'Remove Item',
      '#submit' => ['page_archive_remove_entity_submit'],
    ];

    $form['archive']['view_schedule']['table'][] = [
      'schedule_id' => &$schedule_id,
      'entity_id' => &$entity_id,
      'title' => &$title,
      'edit' => &$schedule_edit,
      'remove' => &$schedule_remove,
    ];


    $form['archive']['view_schedule']['table']['#rows'][] = [
      ['data' => &$entity_id],
      ['data' => &$title],
      ['data' => &$schedule_edit],
      ['data' => &$schedule_remove]
    ];

    unset($title);
    unset($entity_id);
    unset($schedule_id);
    unset($schedule_remove);
    unset($schedule_edit);
  }

  $form['archive']['view_schedule']['pager'][] = [
    '#theme' => 'pager'
  ];
  return $form;
}

/**
 * Handle the submission of edit.
 * @param $form
 * @param $form_state
 */
function page_archive_edit_entity_submit($form, &$form_state) {
  global $base_url;
  $entity_id = $form_state['clicked_button']['#name'];
  drupal_goto("$base_url/admin/archive/archive_entity/manage/edit/$entity_id", ['query' => ['destination' => 'admin/archive/view-schedule']]);
}

/**
 * Handle the submission of deletion of an entity.
 * @param $form
 * @param $form_state
 */
function page_archive_remove_entity_submit($form, &$form_state) {
  $entity_id = page_archive_sid_lookup_entity_id_schedule($form_state['clicked_button']['#name']);
  $delete = page_archive_delete_schedule($form_state['clicked_button']['#name']);
  global $base_url;
  drupal_goto("$base_url/admin/archive/archive_entity/manage/delete/$entity_id", ['query' => ['destination' => 'admin/archive/view-schedule']]);
}
