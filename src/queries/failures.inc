<?php

/**
 * insert any new failures into DB
 *
 * @param $data
 * @return bool|int
 */
function page_archive_insert_failure($data) {
  $return = drupal_write_record('page_archive_failures', $data);
  return $return;
}


/**
 * Check if a failure record exist
 *
 * @param $entity_id
 * @return bool
 */
function page_archive_failure_record_exist($entity_id) {
  try {
    $query = db_select('page_archive_failures', 'e')
      ->fields('e', ['entity_id'])
      ->condition('entity_id', $entity_id)
      ->execute()
      ->fetchAssoc();
  } catch (Exception $e) {
    watchdog('Page Archive', "Record exist in db failed: $e");
  }

  if (!$query) {
    return FALSE;
  }
  return TRUE;
}

/**
 * Delete a failure record from the db
 * @param $entity_id
 * @return bool|int
 */
function page_archive_delete_failure($entity_id) {
  try {
    $delete = db_delete('page_archive_failures')
      ->condition('entity_id', $entity_id)
      ->execute();
  } catch (Exception $e) {
    $delete = FALSE;
    $msg = "DB DELETE ERROR: $e";
    watchdog(
      'Page Archive',
      t($msg),
      NULL,
      WATCHDOG_ERROR
    );
  }
  return $delete;
}

