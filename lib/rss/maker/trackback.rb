require 'rss/trackback'
require 'rss/maker/1.0'

module RSS
  module Maker
    module TrackBackModel
      def self.append_features(klass)
        super

        %w(ping about).each do |element|
          name = "#{RSS::TRACKBACK_PREFIX}_#{element}"
          klass.add_need_initialize_variable(name)
          klass.add_other_element(name)
          klass.__send__(:attr_accessor, name)
          klass.module_eval(<<-EOC, __FILE__, __LINE__)
            def setup_#{name}(rss, current)
              if #{name} and current.respond_to?(:#{name}=)
                current.#{name} = @#{name} if @#{name}
              end
            end
          EOC
        end
      end
    end

    class ItemsBase
      class ItemBase; include TrackBackModel; end
    end
  end
end
