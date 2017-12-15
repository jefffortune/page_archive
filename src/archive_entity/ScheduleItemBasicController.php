  <?php

interface ArchiveEntityBasicControllerInterface
  extends DrupalEntityControllerInterface {

  /**
   * Create an entity.
   */
  public function create();

  /**
   * Save an entity.
   *
   * @param object $entity
   *   The entity to save.
   */
  public function save($entity);

  /**
   * Delete an entity.
   *
   * @param object $entity
   *   The entity to delete.
   */
  public function delete($entity);

}

class ArchiveEntityBasicController
  extends DrupalDefaultEntityController
  implements ArchiveEntityBasicControllerInterface {

  /**
   * Create and return a new entity_example_basic entity.
   */
  public function create() {
    $entity = new stdClass();
    $entity->type = 'archive_entity';
    $entity->id = 0;
    $entity->bundle_type = 'archive_item';
    $entity->title = '';
    $entity->field_archive_url = '';
    $entity->field_archive_selectors = '';
    $entity->field_archive_description= '';
    $entity->field_archive_images = 0;
    return $entity;
  }

  /**
   * Saves the custom fields using drupal_write_record().
   */
  public function save($entity) {
    // If our entity has no id, then we need to give it a
    // time of creation.
    if (empty($entity->id)) {
      $entity->created = time();
    }
    // Invoke hook_entity_presave().
    module_invoke_all('entity_presave', $entity, 'archive_entity');
    // The 'primary_keys' argument determines whether this will be an insert
    // or an update. So if the entity already has an ID, we'll specify
    // id as the key.
    $primary_keys = $entity->id ? 'id' : array();
    // Write out the entity record.
    drupal_write_record('page_archive_entity', $entity, $primary_keys);
    // We're going to invoke either hook_entity_update() or
    // hook_entity_insert(), depending on whether or not this is a
    // new entity. We'll just store the name of hook_entity_insert()
    // and change it if we need to.
    $invocation = 'entity_insert';
    // Now we need to either insert or update the fields which are
    // attached to this entity. We use the same primary_keys logic
    // to determine whether to update or insert, and which hook we
    // need to invoke.
    if (empty($primary_keys)) {
      field_attach_insert('archive_entity', $entity);
    }
    else {
      field_attach_update('archive_entity', $entity);
      $invocation = 'entity_update';
    }
    // Invoke either hook_entity_update() or hook_entity_insert().
    module_invoke_all($invocation, $entity, 'archive_entity');
    return $entity;
  }

  /**
   * Delete a single entity.
   *
   * Really a convenience function for deleteMultiple().
   */
  public function delete($entity) {
    $this->deleteMultiple(array($entity));
  }

  /**
   * Delete one or more schedule_item entities.
   *
   * Deletion is unfortunately not supported in the base
   * DrupalDefaultEntityController class.
   *
   * @param array $entities
   *   An array of entity IDs or a single numeric ID.
   * @throws Exception
   */
  public function deleteMultiple($entities) {
    $ids = array();
    if (!empty($entities)) {
      $transaction = db_transaction();
      try {
        foreach ($entities as $entity) {
          // Invoke hook_entity_delete().
          module_invoke_all('entity_delete', $entity, 'archive_entity');
          field_attach_delete('archive_entity', $entity);
          $ids[] = $entity->id;
        }
        db_delete('page_archive_entity')
          ->condition('id', $ids, 'IN')
          ->execute();
      }
      catch (Exception $e) {
        $transaction->rollback();
        watchdog_exception('Page Archive', $e);
        throw $e;
      }
    }
  }

  public function views_data() {
    $data = parent::views_data();

    return $data;
  }
}