require "rss/0.9"

require "rss/maker/base"

module RSS
  module Maker
    
    class RSS09 < RSSBase
      
      def initialize(rss_version="0.91")
        super
      end
      
      def to_rss
        rss = Rss.new(@rss_version, @version, @encoding, @standalone)
        setup_xml_stylesheets(rss)
        setup_channel(rss)
        setup_other_elements(rss)
        if rss.channel
          rss
        else
          nil
        end
      end
      
      private
      def setup_channel(rss)
        @channel.to_rss(rss)
      end
      
      class Channel < ChannelBase
        
        def to_rss(rss)
          channel = Rss::Channel.new
          set = setup_values(channel)
          if set
            rss.channel = channel
            setup_items(rss)
            setup_image(rss)
            setup_textinput(rss)
            setup_other_elements(rss)
            if rss.channel.image
              rss
            else
              nil
            end
          end
        end
        
        def have_required_values?
          @title and @link and @description and @language and
            @maker.image.have_required_values?
        end
        
        private
        def setup_items(rss)
          @maker.items.to_rss(rss)
        end
        
        def setup_image(rss)
          @maker.image.to_rss(rss)
        end
        
        def setup_textinput(rss)
          @maker.textinput.to_rss(rss)
        end
        
        def variables
          super + ["pubDate"]
        end

        class Cloud < CloudBase
        end

      end
      
      class Image < ImageBase
        def to_rss(rss)
          image = Rss::Channel::Image.new
          set = setup_values(image)
          if set
            image.link = link
            rss.channel.image = image
            setup_other_elements(rss)
          end
        end
        
        def have_required_values?
          @url and @title and link
        end
      end
      
      class Items < ItemsBase
        def to_rss(rss)
          if rss.channel
            normalize.each do |item|
              item.to_rss(rss)
            end
            setup_other_elements(rss)
          end
        end
        
        class Item < ItemBase
          def to_rss(rss)
            item = Rss::Channel::Item.new
            set = setup_values(item)
            if set
              rss.items << item
              setup_other_elements(rss)
            end
          end
          
          private
          def have_required_values?
            @title and @link
          end

          class Guid < GuidBase
            def to_rss(*args)
            end
          end
        
          class Enclosure < EnclosureBase
            def to_rss(*args)
            end
          end
        
          class Source < SourceBase
            def to_rss(*args)
            end
          end
        
          class Category < CategoryBase
            def to_rss(*args)
            end
          end
          
        end
      end
      
      class Textinput < TextinputBase
        def to_rss(rss)
          textInput = Rss::Channel::TextInput.new
          set = setup_values(textInput)
          if set
            rss.channel.textInput = textInput
            setup_other_elements(rss)
          end
        end

        private
        def have_required_values?
          @title and @description and @name and @link
        end
      end
    end
    
    add_maker(filename_to_version(__FILE__), RSS09)
  end
end
