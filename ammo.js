(function ($) {

  Drupal.behaviors.ammo = {
    attach: function (context, settings) {

        $('.tablinks').bind('click',function(e){
          e.preventDefault();
          $('.tabcontent').removeClass('active');
          $('.tablinks.active').removeClass('active');
          $(this).addClass('active');
          var tabId = $(this).attr('Id');
          var contentId = '#tabcontent-' + tabId.substring(4);
          $(contentId).addClass('active');
        });

        // Read more toggle functionality
        $('.text-limit', context).once('read-more').each(function() {
          var $element = $(this);
          var originalHtml = $element.html();
          var originalText = $element.text().trim();
          var maxLength = 280;
          
          if (originalText.length > maxLength) {
            // Store original HTML for later use
            $element.data('original-html', originalHtml);
            
            // Find truncation point in plain text
            var truncatedText = originalText.substring(0, maxLength);
            var remainingText = originalText.substring(maxLength);
            
            // Create proper truncated version with hideable containers
            var truncatedHtml = 
              '<span class="truncated-text">' + truncatedText + 
              '<span class="ellipsis">...</span>' +
              ' <a href="#" class="read-more-link">Lees meer</a></span>' +
              '<span class="more-text" style="display: none;">' + originalHtml + 
              ' <a href="#" class="read-less-link">Lees minder</a></span>';
            
            $element.html(truncatedHtml);
          }
        });
        
        // Handle read more click (compatible with jQuery 1.4.4)
        $(context).delegate('.read-more-link', 'click', function(e) {
          e.preventDefault();
          var $link = $(this);
          $link.closest('.truncated-text').hide();
          $link.closest('.text-limit').find('.more-text').show();
        });
        
        // Handle read less click (compatible with jQuery 1.4.4)
        $(context).delegate('.read-less-link', 'click', function(e) {
          e.preventDefault();
          var $link = $(this);
          $link.closest('.more-text').hide();
          $link.closest('.text-limit').find('.truncated-text').show();
        });

    }
  };

})(jQuery);
