<?php

/**
 * @file
 * Hooks provided by the Page Archive module.
 */

/**
 * Hook that allows you to supply a custom rule tot eh elysia_cron module.
 * More about the Elysia Cron module can be found https://www.drupal.org/project/elysia_cron.
 * The return is a string rule for when the cron job managed by elysia cron should run.
 * @param $rule
 * - String that shows the current rule that is being used by the module
 * @return string
 * - return the rule if you want keep the default settings.
 * - override the rule with your string version to provide your custom interval.
 */
function hook_archive_elysia_schedule($rule) {
  return $rule;
}

/**
 * Hook allows you to create a cron schedule by returning the interval_time or supplying your own interval_time string.
 * Example would be (60 * 60) this would run the archive program every 1 hour.
 * @param $interval_time
 * @return string
 */
function hook_archive_cron_schedule($interval_time) {
  return $interval_time;
}

/**
 * Hook gives you an archive_entity object that you can alter values on prior to the creating
 * the archive page node.
 * @param $entity
 *  - id : entity id
 *  - title : entity title
 *  - field_archive_url: url to be scraped
 *  - field_archive_description: The description of the page
 *  - field_archive_selectors: What html elements will be scraped.
 *  - field_images: 0 for no images to be archived 1 for images to be archived
 *  - field_node: if a valid nid is in this then it referencing the internal node that will be archived.
 * @return object
 */
function hook_archive_presave_internal_entity($entity) {
  return $entity;
}

/**
 * This allows you to customize how you want to set your metatags for the created node
 * @param $internal_node
 *  - node object will be given if internal, false will be given if it external source being archived.
 * @param $archived_node
 *  - Archived node with current values.
 * @param $pulled_metatags
 *  - Metatag array pulled from the scraped html page.
 */
function hook_archive_node_metatag_alter($internal_node, $archived_node, $pulled_metatags) {

}

/**
 * This hook will give you the ability to customize values and add specific values for your site.
 * Very good use for moderation and custom fields, taxonomy and additional information to be updated or manipulated.
 * @param $node
 *  - Archive Page node object at current state.
 * @param $entity
 * - The Archive Entity object that scheduled the archive job.
 */
function hook_archive_created_save_alter($node, $entity) {

}