<?php

use Drupal\page_archive\CSVHandler;

module_load_include('inc', 'page_archive', 'src/classes/CSVHandler');

/**
 * Settings page for manging taxonomy, download CSV shell and uploading and validating CSV files.
 * @param $form
 * @param $form_state
 * @return mixed
 */
function page_archive_admin_csv_upload($form, $form_state) {
  $intro = '
  <h1>Upload CSV</h1>
  <hr style="margin-bottom: 2em" />
  ';

  $form['page_archive']['intro'] = [
    '#id' => 'intro-archive-upload',
    '#markup' => $intro,
  ];
  // Get all attached taxonomy reference from archive page node
  $taxonomy_term_fields = page_archive_get_taxonomy_reference_from_archive_page();

  $default = [];
  $archive_taxonomy = variable_get('archive_taxonomy');
  foreach ($archive_taxonomy as $key => $term) {
    if ($term !== 0) {
      $default[] = $key;
    }
  }

  if (!empty($taxonomy_term_fields)) {
    $form['page_archive']['archive_taxonomy']['archive_taxonomy'] = [
      '#title' => t('Validate Taxonomy Terms'),
      '#type' => 'checkboxes',
      '#description' => t('When selected, validation will check if taxonomy terms exist in the database. If not checked new terms will be added from the csv fields with no validation.'),
      '#options' => $taxonomy_term_fields,
      '#default_value' => $default
    ];

    $form['page_archive']['archive_taxonomy']['submit'] = array(
      '#type' => 'submit',
      '#value' => 'Save Configuration',
      '#submit' => ['system_settings_form_submit'],
    );

  }

  $csv_file = '<div style="margin: 2em 0 2em"> ';
  $csv_file .= '<div><b>Download a custom archived csv based on your archive node type.</b></div>';
  $csv_file .= '<div><b style="color: red">Any changes to Archive Page fields will require a new CSV archive.</b></div>';
  $csv_file .= '</div>';
  $form['page_archive']['archive_upload']['link_to_example']['#markup'] = t($csv_file);
  $form['page_archive']['archive_upload']['submit'] = array(
    '#label' => t('Download the csv archive for archiving.'),
    '#type' => 'submit',
    '#value' => 'CSV archive',
    '#description' => t('If any modification to Archive Page node fields you will need to re run csv archive.'),
    '#submit' => ['page_archive_admin_build_csv'],
  );

  $form['page_archive']['archive_csv_file']['archive_csv_file'] = [
    '#type' => 'managed_file',
    '#title' => t('Upload Archive CSV'),
    '#description' => ($max_size = parse_size(ini_get('upload_max_filesize'))) ? t('Due to server restrictions, the <strong>maximum upload file size is !max_size</strong>.
      Files that exceed this size will be disregarded.', ['!max_size' => format_size($max_size)]) : '',
    '#progress_indicator' => 'bar',
    '#upload_validators' => [
      'file_validate_extensions' => ['csv'],
    ],
  ];


  $form['page_archive']['archive_csv_file']['submit'] = [
    '#type' => 'submit',
    '#value' => 'Validate CSV',
    '#submit' => ['page_archive_csv_import_submit']
  ];

  return $form;

}

/**
 * This is submit handler for when the user submits the CSV in the admin page.
 * Will send it to the CSVHandler object that will verify if the CSV is correct.
 * If valid it will make the entities for scheduling.
 * Not valid it will return the error that was found in the uploaded csv.
 *
 * @param $form
 * @param $form_state
 * @return mixed
 */
function page_archive_csv_import_submit(&$form, &$form_state) {

  $file = file_load($form_state['input']['archive_csv_file']['fid']);
  $csvHandler = new CSVHandler($file->uri);

  if (!empty($csvHandler->getError())) {
    drupal_set_message($csvHandler->getError(), 'error');
    return $form;
  }

  if (!$csvHandler->validateCSV()) {
    form_set_error('CSV Upload', $csvHandler->getError());
    return $form;
  }

  $csvHandler->buildEntityFromRows();
  $msg = 'The uploaded csv file has successfully scheduled your entries.';
  $msg .= ' Check the failures page to see if any corrections need to be completed';
  drupal_set_message($msg);
  return $form;
}


/**
 * When CSV archive button is engaged the system will build the csv with a header with the elements needed
 * to populate archive page content type.
 *
 * @param $form
 * @param $form_state
 */
function page_archive_admin_build_csv($form, $form_state) {
  drupal_add_http_header('Content-Type', 'text/csv');
  drupal_add_http_header('Content-Disposition', 'attachment; filename=page_archive.csv');
  $csv_headers = ['title', 'url', 'selector'];
  // Get any attached referenced taxonomy fields
  $taxonomy_term_fields = page_archive_get_taxonomy_reference_from_archive_page();
  // print each term in a new header cell
  foreach ($taxonomy_term_fields as $taxonomy) {
    $csv_headers[] = $taxonomy;
  }
  $csv_headers[] = 'description';
  $csv_headers[] = 'get images';
  // Print the csv to the stream
  print implode(',', $csv_headers) . PHP_EOL;
  drupal_exit();
}
