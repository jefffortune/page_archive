<?php

function archive_entity_menu() {
  $items = [];
  $items['admin/archive/archive_entity/manage'] = [
    'title' => 'Archive Entity',
    'page callback' => 'archive_entity_list_entities',
    'access arguments' => ['manage archive_entity entity'],
    'weight' => 3
  ];

  // Add example entities.
  $items['admin/archive/archive_entity/manage/add'] = [
    'title' => 'Add ',
    'page callback' => 'archive_entity_add',
    'access arguments' => ['manage archive_entity entity'],
    'type' => MENU_LOCAL_ACTION,
  ];

  // Add example entities.
  $items['admin/archive/archive_entity/manage/edit/%'] = [
    'title' => 'Edit',
    'page callback' => 'archive_entity_edit',
    'page arguments' => [5],
    'access arguments' => ['create archive_entity entity'],
  ];

  // Add example entities.
  $items['admin/archive/archive_entity/manage/delete/%'] = [
    'title' => 'Delete',
    'page callback' => 'drupal_get_form',
    'page arguments' => ['archive_entity_delete_confirm', 5],
    'access arguments' => ['create archive_entity entity'],
    'weight' => 1,
    'type' => MENU_LOCAL_TASK,
    'context' => MENU_CONTEXT_INLINE,
  ];

  // List of all entity_example_basic entities.
  $items['admin/archive/archive_entity/manage/list'] = [
    'title' => 'List',
    'type' => MENU_DEFAULT_LOCAL_TASK,
  ];

  return $items;
}