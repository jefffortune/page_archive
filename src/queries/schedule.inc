<?php

/**
 * Insert the scheduled data that has a reference to the archive schedule entity.
 *
 * @param $data
 *
 * @return bool|int
 */
function page_archive_insert_scheduler($data) {
  $return = drupal_write_record('page_archive_scheduled', $data);
  return $return;
}

/**
 * Check to see if a entry exist in the database using the nid of the node.
 * If it does exist then return the schedule Id if not return false.
 *
 * @param $nid
 *
 * @return mixed
 * return the schedule_id to verify that has been deleted and used for getting
 * the schedule_id from the nid.
 */
function page_archive_nid_lookup_exists_schedule($nid) {
  $exists = db_select('page_archive_scheduled', 'c')
    ->fields('c')
    ->condition('nid', $nid)
    ->execute()
    ->fetchAssoc();

  if ($exists['nid'] == $nid) {
    return $exists['schedule_id'];
  } else {
    return FALSE;
  }
}

/**
 * @param $nid
 *
 * @return bool
 */
function page_archive_nid_lookup_entity_id_schedule($nid) {
  $return = db_select('page_archive_scheduled', 'c')
    ->fields('c')
    ->condition('nid', $nid)
    ->execute()
    ->fetchAssoc();

  if ($return['nid'] == $nid) {
    return $return['entity_id'];
  } else {
    return FALSE;
  }
}

/**
 * Given an entity id return the schedule_id if it exist in the page archive schedule table.
 * @param $entity_id
 * @return bool
 */
function page_archive_lookup_schedule_id_by_entity_id($entity_id) {
  $return = db_select('page_archive_scheduled', 'c')
    ->fields('c')
    ->condition('entity_id', $entity_id)
    ->execute()
    ->fetchAssoc();

  if ($return['entity_id'] == $entity_id) {
    return $return['schedule_id'];
  } else {
    return FALSE;
  }
}


/**
 * Given a schedule id look up in the page archive schedule database and return the entity id.
 * @param $schedule_id
 *
 * @return \Exception
 */
function page_archive_sid_lookup_entity_id_schedule($schedule_id) {
  try {
    $return = db_select('page_archive_scheduled', 's')
      ->fields('s', ['entity_id'])
      ->condition('schedule_id', $schedule_id)
      ->execute()
      ->fetchAssoc();
  } catch (Exception $e) {
    return $e;
  }
  return $return['entity_id'];
}

/**
 * db query to remove the schedule record form the database.
 *
 * @param $schedule_id
 *
 * @return bool|int
 */
function page_archive_delete_schedule($schedule_id) {
  try {
    $delete = db_delete('page_archive_scheduled')
      ->condition('schedule_id', $schedule_id)
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

/**
 * return all schedule id in the page_archive_schedule table.
 * @return \Exception
 */
function page_archive_get_all_schedule_id() {
  try {
    $query = db_select('page_archive_scheduled', 'c')
      ->fields('c', ['schedule_id', 'entity_id'])
      ->execute()
      ->fetchAll();

    return $query;
  } catch (Exception $e) {
    return $e;
  }
}

/**
 * Give a batch amount return the amount of records equaling batch size.
 * @param $batch
 * @return bool
 */
function page_archive_get_batch_schedule($batch) {
  try {
    $query = db_select('page_archive_scheduled', 'c')
      ->fields('c')
      ->range(0, $batch)
      ->execute()
      ->fetchAll();
    return $query;
  } catch (Exception $e) {
    watchdog('Page Archive', $e, NULL, WATCHDOG_ERROR);
    return FALSE;
  }
}

/**
 * Given entity id check if the record exist in the archive page scheduled table.
 * @param $entity_id
 *
 * @return bool
 */
function page_archive_record_exist($entity_id) {
  try {
    $query = db_select('page_archive_scheduled', 'e')
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
