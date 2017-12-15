## Page Archive
### Purpose
The Archive module is intended to take both internal pages from the Drupal CMS and external pages and create an Archived content type inside the Drupal CMS. The initial process is to schedule content to be archived to be managed by the Cron module to archive predetermined batch size during non-peak hours.

### Dependencies
* required in the info:
    * entity
    * pathauto
    * redirect
* nice to have modules:
    * elysia_cron
    * composer_manager
* composer dependencies:
    * paquettg/php-html-parser
    * ezyang/htmlpurifier

### Installation
1. Make sure required modules are downloaded and in module folder.
1. https://www.drupal.org/docs/7/extending-drupal-7/installing-drupal-7-contributed-modules#enable_mod
1. Install composer requirements if composer_manager not installed
    * in the sites root with composer installed run following commands
        * composer require paquettg/php-html-parser
        * composer require ezyang/htmlpurifier
1. Navigate to admin/content/archive/settings and setup the settings for the module

### _**Archive Settings**_

There are few settings that need to be set in order for the Archive module to work properly.

1. Allowed domains field needs to have the full URL’s, for example, [https://one.nhtsa.gov](https://one.nhtsa.gov/). Each new URL needs to be separated by a comma.
2. CSS Selector only applies to internal pages that are being archived.
3. Batch size will run how many schedule entities that will run during the Cron process.
4. Delete the node that is being archived. A checkbox when checked will delete the node that was archived, otherwise it will be removed from publish status.

### _**Scheduling Process**_

Two different ways to schedule an archive job.

1. Node edit form will have a button located in the additional information tabs with a label of Archive. This button will work for both scheduling and removing the node from being archived. If the Node does not exist in the schedule database table it will have the ability to handle scheduling the node to the schedule table. If the node already exists in the node edit schedule it will handle the removal of the node from the scheduled database table.
2. CSV upload form will be located at the path admin/content/archive. Example CSV is available to download on the upload page. After the proper fields in the csv are filled out you can upload the file and then submit the form. The form process will execute the following actions:
    1. Validate the URL with allowed URL’s.
    2. Validate the path exist
    3. Validate the following cells are filled out properly
        1. Title
        2. Selector
        3. Description
        4.  Images
            1. 0 for don’t bring images from content
            2. 1 for bring images from content

        5. Validate Audience, Program, Topic and Tag taxonomy exist in the taxonomy table.

### _**Cron Process**_

Cron is a daemon that executes commands at specified intervals. In the Archive module currently has a hard time interval set to run scheduled jobs. When the requirements are met to run a scheduled job for the archive module it will:

1. Make an Archive object that:
    1. Make a query to the scheduled entities database table for the batch amount set in the settings. It uses the first in first out process.
    2. Process the entity by scraping the given URL.
    3. Based on entity image field pull in or delete images from the content.
    4. Create an Archive entity
    5. Make redirects if necessary
    6. Make path alias for the new Archive page.
    7. Delete the copied node if it applies.
    8. For any reason that a node fails to be produced an Archive node the entity will be moved to a failures table for future editing by the user.

*Only published pages can be archived, the parser is checking the URL of the node being archived. If the parser can not reach the page it will fail the node and insert it into the failures table.

## **Test Cases**

### **_As a Content Editor:_**

**Wanting to archive a single piece of content that that is inside the Drupal CMS:**

1. I have to navigate to the node edit screen. There are two ways to achieve going to node edit screen. Both ways need me to be logged into the Drupal CMS.
    1. I use the admin tool bar and click the content link. I then identity the content I am looking for in the list using filters and searching. When I find the content, I click on the edit button.
    2. I am on the URL of the page I want to archive. I locate the new draft or edit tab located under the site navigation.

2. While on the edit form I scroll to the bottom of the page. I click on a tab in the additional information labeled Archive Content.
3. The button appears to say Archive Content.
4. I click the button to schedule the content to be archived.
5. After the page reloads I get a message that my content was scheduled.

**Wanting to edit a node that I archived using the node edit screen:**

1. Will have to navigate to the scheduled content type. There are two ways to achieve this:
    1. On /admin/content/archive/view-schedule I can click the edit button and it will reroute me to schedule item from.
    2. I can use the admin menu by going to archive &gt;  Manage Schedule &gt;  and locating the item I want to edit and clicking edit button.

2. After making the changes to the node I click save to update the changes to the scheduled content.

**Wanting to delete a scheduled item before the cron job runs:**

1. Will have to navigate to the scheduled content type. There are two ways to achieve this:
    1. On /admin/content/archive/view-schedule I can click the delete button and it will remove the scheduled item information from the archive system.
    2. I can use the admin menu by going to archvie &gt; Manage Schedule &gt;   and locating the item I want to delete and click delete button and it removes the scheduled item from the archive system.

** Wanting to fix an archive item that might have failed**

1. Navigate to /admin/archive/view-failures
2. Press edit to edit the archive item that failed.
    1. Will validate the fields and give error
    2. On success will add the archive item back into the schedule

3. Press delete to remove the item from the archive system

### **_As a Content Admin_**

 **Wanting to make changes to the settings**

1. I have to navigate to /admin/content/archive/settings
    1. I make change to allowed domains:
        1. Has to be a full URL such as [http://somesite.gov](http://somesite.gov) or [https://somesite.com](https://somesite.com)
        2. More than one URL separate them by a comma

    2. Make change to Default CSS Selector. This only adds a default selector to internal content when they are scheduled form node edit screens.
        1. Has to be a valid CSS class, ID or element
            1. .someclass
            2. \#someID

        2. If I am using multiple selectors I need to separate them by a comma

    3. Make changes to a batch size
        1. Enter a batch size
            1. Enter a whole number of scheduled items I want to run over a single cron interval.

        2. After I make the changes I click save button to save the configuration

 **Upload a CSV to be processed into archive items.**

1. Create a csv from the example csv that can be found by clicking the download example.csv link at /admin/content/archive/settings
2. Navigate to /admin/content/archive
3. Press choose file and use the file system to get the csv I need to upload or Drag the csv and drop it on the choose file field.
4. Press upload button next to the choose file button
5. Press save
6. The csv will validate and stop and give you feedback on what cell needs to be fixed.
7. If any errors fix them and try again
8. After the csv is validate get a success message.

## For Developers
Hooks have been added to help you customize the content for your building purposes. Please refer to the page_archive.api.php for documentation.
