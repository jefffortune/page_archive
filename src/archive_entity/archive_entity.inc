<?php

use Drupal\page_archive\CSVHandler;

module_load_include('inc', 'page_archive', 'src/classes/CSVHandler');
module_load_include('inc', 'page_archive', 'src/queries/schedule');
module_load_include('inc', 'page_archive', 'src/queries/failures');

function archive_entity_info() {
  $info = [
    // A human readable label to identify our entity.
    'label' => t('Archive Entity'),

    // The controller for our Entity, extending the Drupal core controller.
    'controller class' => 'ArchiveEntityBasicController',

    // The table for this entity defined in hook_schema()
    'base table' => 'page_archive_entity',

    // Returns the uri elements of an entity.
    'uri callback' => 'archive_entity_uri',

    // IF fieldable == FALSE, we can't attach fields.
    'fieldable' => TRUE,

    // entity_keys tells the controller what database fields are used for key
    // functions. It is not required if we don't have bundles or revisions.
    // Here we do not support a revision, so that entity key is omitted.
    'entity keys' => [
      // The 'id' (id here) is the unique id.
      'id' => 'id',
      // Bundle will be determined by the 'bundle_type' field.
      'bundle' => 'bundle_type',
    ],
    'bundle keys' => [
      'bundle' => 'bundle_type',
    ],

    // FALSE disables caching. Caching functionality is handled by Drupal core.
    'static cache' => TRUE,

    // Bundles are alternative groups of fields or configuration
    // associated with a base entity type.
    'bundles' => [
      'archive_item' => [
        'label' => 'Archive Item',
        // 'admin' key is used by the Field UI to provide field and
        // display UI pages.
        'admin' => [
          'path' => 'admin/archive/archive_entity/manage',
          'access arguments' => ['administer archive_entity entities'],
        ],
      ],
    ],
  ];

  return $info;
}

/**
 * Implements the uri callback.
 */

function archive_entity_uri($basic) {
  return [
    'path' => 'archive_entity/' . $basic->id
  ];
}

function archive_entity_load_multiple($ids = FALSE, $conditions = array(), $reset = FALSE) {
  return entity_load('archive_entity', $ids, $conditions, $reset);
}

/**
 * Callback for a page title when this entity is displayed.
 */
function archive_entity_title($entity) {
  return t('(title=@title)', ['@title' => $entity->title]);
}

/**
 * Provides a wrapper on the edit form to add a new entity.
 */
function archive_entity_add() {
  // Create a basic entity archive to be used and passed to the validation
  // and submission functions.
  $entity = entity_get_controller('archive_entity')->create();

  return drupal_get_form('archive_entity_form', $entity);
}

/**
 * Returns a render array with all entity_example_basic entities.
 *
 * In this basic example we know that there won't be many entities,
 * so we'll just load them all for display. See pager_example.module
 * to implement a pager. Most implementations would probably do this
 * with the contrib Entity API module, or a view using views module,
 * but we avoid using non-core features in the Examples project.
 *
 * @see pager_example.module
 */
function archive_entity_list_entities() {
  $content = array();
  // Load all of our entities.
  $entities = archive_entity_load_multiple();
  if (!empty($entities)) {
    foreach ($entities as $entity) {
      // Create tabular rows for our entities.
      $edit = l('Edit', "admin/archive/archive_entity/manage/edit/$entity->id", ['attributes' => ['class' => ['button']]]);
      $delete = l('Delete', "admin/archive/archive_entity/manage/delete/$entity->id", ['attributes' => ['class' => ['button']]]);
      $rows[] = array(
        'data' => array(
          'id' => $entity->id,
          'item_description' => l($entity->title, 'admin/archive/archive_entity/manage/edit/' . $entity->id),
          'actions' => $edit . $delete,
        ),
      );
    }
    // Put our entities into a themed table. See theme_table() for details.
    $content['entity_table'] = array(
      '#theme' => 'table',
      '#rows' => $rows,
      '#header' => array(t('ID'), t('Title'), t('Actions')),
    );
  } else {
    // There were no entities. Tell the user.
    $content[] = array(
      '#type' => 'item',
      '#markup' => t('No Archive Entities currently exist.'),
    );
  }
  return $content;
}


/**
 * Form function to create an entity_example_basic entity.
 *
 * The pattern is:
 * - Set up the form for the data that is specific to your
 *   entity: the columns of your base table.
 * - Call on the Field API to pull in the form elements
 *   for fields attached to the entity.
 */
function archive_entity_form($form, $form_state, $entity) {
  $form['title'] = [
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#required' => TRUE,
    '#weight' => -5,
    '#default_value' => $entity->title,
  ];

  $form['field_archive_url'] = [
    '#title' => t('URL'),
    '#description' => t('The full url of the page that is being archive.'),
    '#required' => TRUE,
    '#weight' => -4,
    '#type' => 'textarea',
    '#default_value' => $entity->field_archive_url,
  ];

  $form['field_archive_selectors'] = [
    '#title' => t('CSS Selectors'),
    '#description' => t('Comma separated list of css selectors. The sections will be imported in in order as listed.'),
    '#required' => TRUE,
    '#weight' => -3,
    '#type' => 'textarea',
    '#default_value' => $entity->field_archive_selectors,

  ];

  $form['field_archive_description'] = [
    '#title' => t('Description'),
    '#description' => t('Used for setting the description if the page if you have metatags enabled.'),
    '#required' => TRUE,
    '#weight' => -2,
    '#type' => 'textarea',
    '#default_value' => $entity->field_archive_description,
  ];

  $form['field_archive_images'] = [
    '#title' => t('Images'),
    '#description' => t('When checked they archive system will also try to bring in images from scraped pages.'),
    '#required' => FALSE,
    '#weight' => -1,
    '#type' => 'checkbox',
    '#default_value' => $entity->field_archive_images,
  ];

  $form['basic_entity'] = [
    '#type' => 'value',
    '#value' => $entity,
  ];

  field_attach_form('archive_entity', $entity, $form, $form_state);

  $form['#validate'][] = 'archive_entity_validate';

  $form['submit'] = [
    '#type' => 'submit',
    '#value' => t('Save'),
    '#submit' => array('archive_entity_form_submit'),
    '#weight' => 100,
  ];

  $form['delete'] = [
    '#type' => 'submit',
    '#value' => t('Delete'),
    '#submit' => array('archive_entity_delete'),
    '#weight' => 200,
  ];

  return $form;
}

function archive_entity_validate($form, &$form_state) {
  $validator = new CSVHandler();
  // Check if field description is blank
  if (!$validator->isCellEmpty($form_state['values']['field_archive_description'])) {
    form_set_error('URL', "Description field cannot be empty");
  }
  //check if the URL is valid.
  $l = $validator->validateURL($form_state['values']['field_archive_url']);
  if (!$validator->validateURL($form_state['values']['field_archive_url'])) {
    form_set_error('URL', "URL is not valid | " . $validator->getError());
  }
  // Check if the css selector is a valid css selector
  if (!$validator->validateSelectors($form_state['values']['field_archive_selectors'])) {
    form_set_error('CSS Selectors', $validator->getError());
  }
}

/**
 * Form submit handler: Submits basic_add_form information.
 */
function archive_entity_form_submit($form, &$form_state) {
  $entity = $form_state['values']['basic_entity'];
  $entity->title = $form_state['values']['title'];
  $entity->field_archive_url = $form_state['values']['field_archive_url'];
  $entity->field_archive_description = $form_state['values']['field_archive_description'];
  $entity->field_archive_selectors = $form_state['values']['field_archive_selectors'];
  $entity->field_archive_images = $form_state['values']['field_archive_images'];
  field_attach_submit('archive_entity', $entity, $form, $form_state);
  $entity = archive_entity_save($entity);
  $data = [
    'entity_id' => $entity->id,
    'uid' => 1,
    'created' => (int)REQUEST_TIME,
    'error' => 0,
    'nid' => NULL,
  ];

  if (page_archive_failure_record_exist($entity->id)) {
    page_archive_delete_failure($entity->id);
  }

  if (!page_archive_lookup_schedule_id_by_entity_id($entity->id)) {
    page_archive_insert_scheduler($data);
  }


  drupal_set_message("Archive Entity $entity->title has been saved.");
  $form_state['redirect'] = 'admin/archive/archive_entity/manage';
}

function archive_entity_edit($param) {
  $entity = entity_load_single('archive_entity', $param);
  return drupal_get_form('archive_entity_form', $entity);
}

/**
 * We save the entity by calling the controller.
 */
function archive_entity_save($entity) {
  return entity_get_controller('archive_entity')->save($entity);

}

function archive_entity_delete_confirm($form, &$form_state, $id) {
  $entity = entity_load_single('archive_entity', $id);
  $form['#entity'] = $entity;
  $form['#id'] = array('#type' => 'value', '#value' => $entity->id);
  $form['#submit'][] = "archive_entity_delete";
  return confirm_form($form,
    t('Are you sure you want to delete %title?', array('%title' => $entity->title)),
    "admin/archive/archive_entity/manage/edit/$entity->id",
    t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel')
  );
}

/**
 * Form deletion handler.
 *
 * @todo: 'Are you sure?' message.
 */
function archive_entity_delete($form, &$form_state, &$entity) {
  if (isset($form_state['values']['basic_entity'])) {
    $entity = $form_state['values']['basic_entity'];
  } else {
    $entity = $form['#entity'];
  }

  archive_entity_delete_entity($entity);

  drupal_set_message(t('The archive entity %title has been deleted.',
      ['%title' => $entity->title])
  );

  $destination = drupal_get_destination();

  if (!empty($destination) && $destination['destination'] !== current_path()) {
    $form_state['redirect'] = $destination['destination'];
  } else {
    $form_state['redirect'] = 'admin/archive/archive_entity/manage';
  }
}

/**
 * Use the controller to delete the entity.
 */
function archive_entity_delete_entity($entity) {
  entity_get_controller('archive_entity')->delete($entity);
}
