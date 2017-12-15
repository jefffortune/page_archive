<?php

namespace Drupal\page_archive;

use EntityFieldQuery;

/**
 * Class CSVHandler
 * @package Drupal\page_archive
 */
class CSVHandler
{

  private $csv;
  private $rows = [];
  private $rowIsValid = TRUE;
  private $error;
  // taxonomy terms
  private $loaded_terms = [];
  private $term_names = [];
  private $archive_taxonomy_settings = [];

  /**
   * CSVHandler constructor.
   * @param null $csv
   */
  public function __construct($csv = NULL) {
    $this->csv = $csv;

    if ($csv != NULL) {
      $this->term_names = $this->taxonomy_reference_from_archive_page();
      $this->archive_taxonomy_settings = variable_get('archive_taxonomy', []);
      $this->setRows();

      if (!empty($this->term_names)) {
        foreach ($this->term_names as $key => $term_name) {
          if (array_key_exists($key, $this->archive_taxonomy_settings) && $this->archive_taxonomy_settings[$key] !== 0) {
            $this->loaded_terms[$term_name] = $this->vocabularyTermsByName($term_name);
          }
        }
      }
    }
  }

  /**
   * Build entities from the csv rows.
   * @param bool $rows
   */
  public function buildEntityFromRows($rows = FALSE) {
    global $user;

    if (!$rows) {
      $rows = $this->rows;
    }

    foreach ($rows as $row) {
      $new_schedule = entity_get_controller('archive_entity')->create();
      $new_schedule->title = $row['title'];
      $new_schedule->field_archive_url = $row['url'];
      $new_schedule->field_archive_selectors = $row['selector'];
      $new_schedule->field_archive_description = $row['description'];
      // Loop through all attached terms and put the vales in attached field storage.
      if (!empty($this->term_names)) {
        foreach ($this->term_names as $key => $term_name) {
          if (!empty($term_name)) {
            $terms = $this->get_term_list_of_tid($row[$term_name], $term_name, $key);
            if(!empty($terms)) {
              $new_schedule->{$key} = ['und' => $terms];
            }
          }
        }
      }

      $new_schedule->field_archive_images = $row['get images'];
      // save the new entity
      $entity = entity_get_controller('archive_entity')->save($new_schedule);

      $data = [
        'entity_id' => $entity->id,
        'uid' => 1,
        'created' => (int)REQUEST_TIME,
        'error' => 0,
        'nid' => NULL,
      ];

      $scheduled = page_archive_insert_scheduler($data);

      if (!$scheduled) {
        $message = 'failed to archive - location CSVHandler->buildEntityFormRows Archive item id: ' . $new_schedule->id;
        watchdog('Page Archive', $message, null, WATCHDOG_ERROR);
      }
    }
  }

  /**
   * Using the taxonomy name getting the term by name and if it not validated and doesn't exist then create term.
   * @param $terms
   * @param $vocabulary
   * @return array
   */
  private function get_term_list_of_tid($terms, $vocabulary, $term_key) {
    $tids = [];
    if (!empty($terms)) {
      $terms = explode(',', $terms);
      foreach ($terms as $term) {
        $taxTerm = taxonomy_get_term_by_name($term, $vocabulary);
        if (!empty($taxTerm)) {
          $taxTerm = array_values($taxTerm);
          $tids[] = ['tid' => $taxTerm[0]->tid];
        } else if ($this->archive_taxonomy_settings[$term_key] === 0) {
          $new_term = $this->create_taxonomy_term(trim($term), $vocabulary);
          if ($new_term) {
            $tids[] = ['tid' => $new_term];
          }

        }
      }
    }

    return $tids;
  }

  /**
   * create a taxonomy term and return the tid
   *
   * @param $name
   * @param $vid
   * @param int $parent_id
   * @return mixed
   */
  public function create_taxonomy_term($name, $vid,$parent_id = 0) {
    $vocabulary_info = taxonomy_vocabulary_machine_name_load($vid);
    $matched_terms = taxonomy_get_term_by_name($name, $vocabulary_info->machine_name);
    if(empty($matched_terms)) {
      $term = new \stdClass();
      $term->name = $name;
      $term->vid = $vocabulary_info->vid;
      $term->parent = array($parent_id);
      taxonomy_term_save($term);
      return $term->tid;
    }
  }

  /**
   * Validate the uploaded CSV and report if any errors are found.
   * @return bool
   */
  public function validateCSV() {
    foreach ($this->rows as $i => $row) {
      if (!$this->isCellEmpty($row['title'])) {
        $this->error .= " Occurred on line " . ($i + 2) . ' in cell title. ';
        return FALSE;
      }

      if (!$this->validateURL($row['url'])) {
        $this->error .= " Occurred on line " . ($i + 2) . ' in cell url.';
        return FALSE;
      }

      if (!$this->validateSelectors($row['selector'])) {
        $this->error .= " Occurred on line " . ($i + 2) . " in cell selector.";
        return FALSE;
      }

      foreach ($this->term_names as $key => $term_name) {
        if ($this->archive_taxonomy_settings[$key] !== 0) {
          if (!$this->validateTerms($row[$term_name], $this->loaded_terms[$term_name])) {
            $this->error .= " Occurred on line " . ($i + 2) . " in cell $term_name.";
            return FALSE;
          }
        }
      }

      if (!$this->isCellEmpty($row['description'])) {
        $this->error .= " Occurred on line " . ($i + 2) . ' in cell description. ';
        return FALSE;
      }

      if (empty($row['get images'])) {
        $row['get images'] = 0;
      }

      if ($row['get images'] === 1 || $row['get images'] === 0) {
        $this->error = "Images need to be 1 or 0. Number 1 is used to demonstrate true and 0 for false.";
      }
    }
    return TRUE;
  }

  /**
   * Check to see if a cell is empty.
   * @param $string
   * @return bool
   */
  public function isCellEmpty($string) {
    if (empty($string)) {
      $this->setError("Cell is required to have a valid entry.");
      return FALSE;
    }
    return TRUE;
  }

  /**
   * Check if URL domain name is in the valid domain list.
   * See if we can get a 200 return from the url.
   * @param $url
   *
   * @return bool
   *  return true if it get a 200 and in the valid domain list.
   * return false if it not in domain list or fails to get a 200
   */
  public function validateURL($url) {
    $allowed_domains = explode(',', variable_get('archive_allowed_domains'));

    // Check if any values are in the valid domain list.
    if (!variable_get('archive_allowed_domains')) {
      $this->error = "Need to set allowed domains in the settings tab.";
      return FALSE;
    }

    $parse = parse_url($url); // split the url up into categories of structure
    // Check the path against the allowed domain list.
    $found_url = FALSE; // Bool variable to hold value if url was found in allowed_domains list
    // Loop through all allowed domains and check for valid value.
    foreach ($allowed_domains as $allowed_domain) {
      if(($parse['scheme'] . '://' . $parse['host']) == trim($allowed_domain)) {
        // If the domains match move bool to TRUE and break loop.
        $found_url = TRUE;
        break;
      }
    }
    // If the found_url value is still false we need to throw an error.
    if (!$found_url) {
      $this->error = "$url not in the allowed domains list.";
      return FALSE;
    }

    $url_headers = get_headers($url);
    // if did get headers back or did not get a 200 status then fail the url validation
    if (!$url_headers || strpos($url_headers[0], '404')) {
      $this->error = "$url is not a valid url.";
      return FALSE;
    }
    // Url got a 200 and is in the allowed domain list return true
    return TRUE;
  }

  /**
   * Check if it can possibly be a csv class or id selector.
   * @param $selectors
   *
   * @return bool
   *  return true if it passes
   *  return false if it failed
   */
  public function validateSelectors($selectors) {
    $array = explode(',', $selectors);
    foreach ($array as $item) {
      if (!$item[0] == '.' || !$item[0] == '#') {
        $this->error = "$item is not a valid css selector.";
        return FALSE;
      }
    }
    return TRUE;
  }

  /**
   * Validate the the loaded vocabulary terms exist in the database already.
   * @param $csvTerms
   * @param $loadedTerms
   *
   * @return bool|null
   * Return true if cell is empty
   * Return true if terms match
   * Return false if terms do not match.
   */
  private function validateTerms($csvTerms, $loadedTerms) {
    // return true if cell is empty
    if (empty($csvTerms)) {
      return TRUE;
    }
    $csvTerms = explode(',', $csvTerms); // load cell into an array
    $match = []; // Bool array for storing if a term matched

    foreach ($csvTerms as $term) {
      if (in_array(trim($term), $loadedTerms)) {
        // If there is a match store true
        $match[] = TRUE;
      } else {
        // Else store false
        $match[] = FALSE;
      }
    }
    // Check for any false values in the array
    if (in_array(FALSE, $match, TRUE)) {
      $no_match = [];
      // Store the name values for any failed match
      foreach ($match as $i => $item) {
        if (!$item) {
          $no_match[] = $csvTerms[$i];
        }
      }
      // Build error message multiple vs single.
      if (sizeof($no_match) > 1) {
        $this->error = "No match for the following terms: ";
        foreach ($no_match as $term) {
          if ($term === end($no_match)) {
            $this->error .= "$term";
          } else {
            $this->error .= "$term, ";
          }
        }
      } else {
        $this->error = "No match for term: " . $no_match[0];
      }
      // Return false because a or some terms didn't match.
      return FALSE;
    }
    // Terms matched return true
    return TRUE;
  }

  /**
   * Set ini_set to detect line endings in the csv upload.
   * Will load csv into an associated array given the key name is the column cell title.
   */
  private function setRows() {
    // Have php aut detect the new line entry for the csv
    ini_set('auto_detect_line_endings', true);
    // Load the csv into a row based on new lines.
    $array = array_map('str_getcsv', file($this->csv));
    // Check to make sure there is an array with values.
    if (!empty($array)) {
      // Remove the first row and store them as headers for the use of keys.
      $header = array_shift($array);
      //  Loop through and assign the header keys to the rows
      foreach ($array as $i => $item) {
        $this->rows[] = array_combine($header, $item);
      }
      // In case of an error happens notify the user to try again and verify the CSV.
    } else {
      $this->error = 'Error in parsing the csv. Please verify fields are correctly';
      $this->error .= ' populated and try again.';
      // Delete the file form the file from the managed file structure.
      file_delete($this->csv);
    }
  }

  /**
   * Find any taxonomy references that are attached to archive_page node.
   * @return array
   * Return an array of taxonomy reference field names as key and vocabulary name as value.
   */
  public function taxonomy_reference_from_archive_page() {
    // load all field instances from teh archive page
    $data_instances = field_info_instances("node", 'archive_page');
    // storage for term fields
    $taxonomy_term_fields = [];
    // loop through all data instances
    foreach ($data_instances as $key => $data_instance) {
      // If the field type is a taxonomy term then add it to the references.
      if (field_info_field($key)['type'] == 'taxonomy_term_reference') {
        $taxonomy_term_fields[$key] = field_info_field($key)['settings']['allowed_values'][0]['vocabulary'];
      }
    }

    return $taxonomy_term_fields;
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

  /**
   * Takes the machine name of the taxonomy term and queries the database for all taxonomy
   * of with the attached vocabulary ID. It then loops through the results and store the
   * name of each term.
   *
   * @param $machine_name
   *
   * @return array|bool
   * Returns array of names or if empty returns false.
   */
  public function vocabularyTermsByName($machine_name) {
    $vocab = taxonomy_vocabulary_machine_name_load($machine_name);
    $query = new EntityFieldQuery();
    $query->entityCondition('entity_type', 'taxonomy_term')
      ->entityCondition('bundle', $machine_name)
      ->propertyCondition('vid', $vocab->vid);
    $results = $query->execute();

    $nameArray = [];
    if (!empty($results['taxonomy_term'])) {
      foreach ($results['taxonomy_term'] as $tid) {
        $term = taxonomy_term_load($tid->tid);
        $nameArray[] = $term->name;
      }
    }
    if (empty($nameArray)) {
      return FALSE;
    }
    unset($results);
    return $nameArray;
  }

}
