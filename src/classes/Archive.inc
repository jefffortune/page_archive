<?php
/**
 * Created by PhpStorm.
 * User: jfortune
 * Date: 11/14/17
 * Time: 3:47 PM
 */

namespace Drupal\page_archive;

use Drupal\Driver\Exception\Exception;

module_load_include('inc', 'page_archive', 'src/queries/schedule');
module_load_include('inc', 'page_archive', 'src/queries/failures');
module_load_include('inc', 'page_archive', 'src/classes/ArchiveContentScraper');
module_load_include('inc', 'page_archive', 'src/classes/CSVHandler');

/**
 * Class Archive
 *
 * @package Drupal\page_arcive
 */
class Archive
{
  // Private variables
  private $amount;
  private $content;
  private $metatags;
  // Public variables
  public $error;

  /**
   * Archive constructor.
   */
  public function __construct() {
    $this->setAmount();

    if ($this->amount == FALSE || sizeof($this->amount) > 0) {
      return FALSE;
    }

    return TRUE;
  }

  /**
   * Method that will decide how a archive content type will be made.
   */
  public function archive() {
    if (is_array($this->amount)) {
      foreach ($this->amount as $item) {
        $nid = $item->nid;
        if ($nid != NULL && $nid != 0) {
          $this->archiveHandler($item->nid, $item->entity_id);
        } else {
          $this->archiveHandler(FALSE, $item->entity_id);
        }
      }
    } else {
      $msg = "Could not read the schedule as an array.";
      watchdog('Page Archive', $msg, NULL, WATCHDOG_ERROR);
    }
  }

  /**
   * Archive the content from an internal page.
   * @param $nid
   * @param $entity_id
   *
   * @return null
   */
  private function archiveHandler($nid, $entity_id) {
    global $base_url;

    $entity = entity_load_single('archive_entity', $entity_id);
    if ($nid) {
      // Load original node.
      $node = node_load($nid);
      // Check if the node is of the workbench moderation system.
      if (isset($node->workbench_moderation['published'])) {
        // if publish exist the replace the node with the published node.
        $node = node_load($node->workbench_moderation['published']->nid);
      }
    }

    // Store content in a private global variable so we don't waste memory passing it.
    $this->scrape(
      $entity->field_archive_url,
      explode(',', $entity->field_archive_selectors),
      $entity_id,
      $entity->field_archive_images
    );

    if (!$this->getError()) {
      // Create a archive node with node data.
      $archive_node = $this->createNode($entity);
      // load the new node
      $archive_node = node_load($archive_node);

      // Check if any modules have alter for metatags.
      $module_implements = module_implements('archive_node_metatag_alter');
      if (!empty($module_implements)) {
        foreach ($module_implements as $module) {
          if (isset($node)) {
            module_invoke($module, 'archive_node_metatag_alter', $node, $archive_node, $this->metatags);
          } else {
            module_invoke($module, 'archive_node_metatag_alter', FALSE, $archive_node, $this->metatags);
          }
        }
      }

      $redirect = [
        'from' => $entity->field_archive_url,
        'to' => drupal_get_path_alias('node/' . $archive_node->nid),
      ];

      // Check if any modules have alter for metatags.
      $module_implements = module_implements('archive_node_insert_redirect');
      if (!empty($module_implements)) {
        foreach ($module_implements as $module) {
          $redirect = module_invoke($module, 'archive_node_insert_redirect', $redirect);
        }
      }

      // Check to make sure that redirect values are not null.
      if ($redirect['from'] != NULL && $redirect['to'] != NULL) {
        // Set redirect
        $this->insertRedirect(
          $redirect['from'],
          $redirect['to']
        );
      }

      // update schedule entity
      $entity->field_archive_success = 1;
      entity_get_controller('archive_entity')->save($entity);

      if (variable_get('archive_delete_node', 0) === 0) {
        $node->status = 0;
        node_save($node);
      } else if (!empty($entity->field_nid)) {
        node_delete($nid);
      }

    }
  }

  /**
   * @param $url
   * @param array $selectors
   * @param $entity_id
   * @param bool $images
   * @return bool|null
   */
  private function scrape($url, array $selectors, $entity_id, $images = TRUE) {
    global $user;
    // Create a scraper object.
    $scraper = new \Drupal\page_archive\ArchiveContentScraper($url, $selectors, $images);
    // Check if error occurred while building the object.
    if (!$scraper->isError()) {
      // Build content
      $scraper->buildContent();
      if ($scraper->isError()) {
        $data = [
          'entity_id' => $entity_id,
          'uid' => $user->uid,
          'created' => (int)REQUEST_TIME,
          'error' => 1,
          'nid' => NULL,
        ];
        $this->reportFailure($data);
        // unset the object if error happens.
        unset($scraper);
        return FALSE;
      }
    } else {
      $this->setError(TRUE);

      $data = [
        'entity_id' => $entity_id,
        'uid' => $user->uid,
        'created' => (int)REQUEST_TIME,
        'error' => 1,
        'nid' => NULL,
      ];

      $this->reportFailure($data);
      // Remove the scraper object for garbage collection.
      unset($scraper);
      // Return false because it did failed at some point in scraping the content
      return FALSE;
    }
    // Get metatags from head of the html document.
    $this->metatags = $scraper->getmetaTags();
    // Store in private variable to be conservative on memory.
    $this->content = $scraper->getContent();
    unset($scraper);
    return NULL;
  }

  private function reportFailure($data) {
    // Check if the entity already stored in the list.
    // If one is there for w/e reason lets over ride the values
    // If not apply the values to a new row.
    if (page_archive_failure_record_exist($data['entity_id'])) {
      drupal_write_record('page_archive_failures', $data['entity_id']);
    } else {
      drupal_write_record('page_archive_failures', $data);
    }
    // Delete from Schedule so doesn't try run again.
    try {
      db_delete('page_archive_scheduled')
        ->condition('entity_id', $data['entity_id'])
        ->execute();
    } catch (Exception $e) {
      watchdog(
        'Page Archive',
        "Could not delete" . $data['entity_id'] . "from schedule database: $e",
        NULL,
        WATCHDOG_ERROR
      );
    }
  }

  /**
   *
   * @param $entity
   * @return \EntityMetadataWrapper
   */
  private function createNode($entity) {
    global $user;

    $values = [
      'type' => 'archive_page',
      'uid' => $user->uid,
      'status' => 1,
      'comment' => 1,
      'promote' => 0,
    ];
    // create the basic node object
    $node = entity_create('node', $values);
    $node->path['pathauto'] = TRUE; // set to true to use pathauto module for path making.
    node_save($node); // Save the node to get the attached fields populated.
    $wrapper = entity_metadata_wrapper('node', $node); // wrap the node for getter and setters.
    // Set the node with the node_data
    $wrapper->title->set($entity->title);
    $wrapper->archive_body->set($this->content);
    $archive_handler = new CSVHandler();
    $taxonomy_term_fields = $archive_handler->taxonomy_reference_from_archive_page();

    //Loop through all attached taxonomy
    foreach ($taxonomy_term_fields as $key => $term_field) {
      // Use the key for the field name and grab all associated taxonomy id's
      $tids = [];
      if (isset($entity->{$key}['und'])) {
        $tids = $entity->{$key}['und'];
      }
      // Storage for tids when pulled from the tids array;
      $tids_values = [];
      // Loop through all tid and
      if ($tids !== null) {
        foreach ($tids as $tid) {
          $tids_values[] = $tid['tid'];
        }
      }
      if ($tids_values !== 0) {
        $wrapper->{$key}->set($tids_values);
      }

    }
    // Save the node
    $wrapper->save();

    // Check if any modules have alter the node.
    $module_implements = module_implements('archive_created_save_alter');
    if (!empty($module_implements)) {
      foreach ($module_implements as $module) {
        module_invoke($module, 'archive_created_save_alter', $node, $entity);
      }
    }

    // remove form the entity from scheduled.
    db_delete('page_archive_scheduled')
      ->condition('entity_id', $entity->id)
      ->execute();

    // return the new node wrapper.
    return $wrapper->nid->value();
  }

  /**
   * Add a redirect to the archived node if it came from an internal scrape.
   * @param $from
   * @param $to
   */
  private function insertRedirect($from, $to) {
    // Create a new redirect object.
    $redirect = new \stdClass();
    // Pass the object into the redirect object prepare statement.
    redirect_object_prepare($redirect);
    // Assign the from.
    $redirect->source = $from;
    // Assign the to.
    $redirect->redirect = $to;
    // Get a redirect hash.
    redirect_hash($redirect);
    // Check if it is a an existing hash
    $existing = redirect_load_by_hash($redirect->hash);
    // If the hash checkout then save the redirect.
    if (!$existing) {
      redirect_save($redirect);
    } else {
      watchdog(
        'Page Archive',
        'Failed to make redirect. Redirect already exist in table.',
        NULL,
        WATCHDOG_NOTICE
      );
    }
  }

  /**
   * @return mixed
   */
  public function getAmount() {
    return $this->amount;
  }

  /**
   *  Will use the schedule database table to grab up to the batch amount of records.
   */
  private function setAmount() {
    $this->amount = page_archive_get_batch_schedule(variable_get('archive_batch_amount', 20));
  }

  /**
   * @return mixed
   */
  private function getSettings() {
    return $this->settings;
  }


  /**
   * @return mixed
   */
  public function getError() {
    return $this->error;
  }

  /**
   * @param mixed $error
   */
  public function setError($error) {
    $this->error = $error;
  }

}
