#
#   finalizer.rb - 
#   	$Release Version: 0.2$
#   	$Revision: 1.1.1.2.2.2 $
#   	$Date: 1998/01/19 05:08:24 $
#   	by Keiju ISHITSUKA
#
# --
#
#   Usage:
#
#   add(obj, dependant, method = :finalize, *opt)
#   add_dependency(obj, dependant, method = :finalize, *opt)
#	��¸�ط� R_method(obj, dependant) ���ɲ�
#
#   delete(obj, dependant, method = :finalize)
#   delete_dependency(obj, dependant, method = :finalize)
#	��¸�ط� R_method(obj, dependant) �κ��
#   delete_all_dependency(obj, dependant)
#	��¸�ط� R_*(obj, dependant) �κ��
#   delete_by_dependant(dependant, method = :finalize)
#	��¸�ط� R_method(*, dependant) �κ��
#   delete_all_by_dependant(dependant)
#	��¸�ط� R_*(*, dependant) �κ��
#   delete_all
#	���Ƥΰ�¸�ط��κ��.
#
#   finalize(obj, dependant, method = :finalize)
#   finalize_dependency(obj, dependant, method = :finalize)
#	��¸��Ϣ R_method(obj, dependtant) �Ƿ�Ф��dependant��
#	finalize����.
#   finalize_all_dependency(obj, dependant)
#	��¸��Ϣ R_*(obj, dependtant) �Ƿ�Ф��dependant��finalize����.
#   finalize_by_dependant(dependant, method = :finalize)
#	��¸��Ϣ R_method(*, dependtant) �Ƿ�Ф��dependant��finalize����.
#   fainalize_all_by_dependant(dependant)
#	��¸��Ϣ R_*(*, dependtant) �Ƿ�Ф��dependant��finalize����.
#   finalize_all
#	Finalizer����Ͽ��������Ƥ�dependant��finalize����
#
#   safe{..}
#	gc����Finalizer����ư����Τ�ߤ��.
#
#

module Finalizer
  RCS_ID='-$Header: /home/cvsroot/ruby/lib/finalize.rb,v 1.1.1.2.2.2 1998/01/19 05:08:24 matz Exp $-'

  # Dependency: {id => [[dependant, method, opt], ...], ...}
  Dependency = {}

  # ��¸�ط� R_method(obj, dependant) ���ɲ�
  def add_dependency(obj, dependant, method = :finalize, *opt)
    ObjectSpace.call_finalizer(obj)
    assoc = [dependant, method, opt]
    if dep = Dependency[obj.id]
      dep.push assoc
    else
      Dependency[obj.id] = [assoc]
    end
  end
  alias add add_dependency

  # ��¸�ط� R_method(obj, dependant) �κ��
  def delete_dependency(obj, dependant, method = :finalize)
    id = obj.id
    for assoc in Dependency[id]
      assoc.delete_if do |d,m,*o|
	d == dependant && m == method
      end
      Dependency.delete(id) if assoc.empty?
    end
  end
  alias delete delete_dependency

  # ��¸�ط� R_*(obj, dependant) �κ��
  def delete_all_dependency(obj, dependant)
    id = obj.id
    for assoc in Dependency[id]
      assoc.delete_if do |d,m,*o|
	d == dependant
      end
      Dependency.delete(id) if assoc.empty?
    end
  end

  # ��¸�ط� R_method(*, dependant) �κ��
  def delete_by_dependant(dependant, method = :finalize)
    method = method.intern unless method.kind_of?(Integer)
    for id in Dependency.keys
      delete(id, dependant, method)
    end
  end

  # ��¸�ط� R_*(*, dependant) �κ��
  def delete_all_by_dependant(dependant)
    for id in Dependency.keys
      delete_all_dependency(id, dependant)
    end
  end

  # ��¸��Ϣ R_method(id, dependtant) �Ƿ�Ф��dependant��finalize��
  # ��.
  def finalize_dependency(id, dependant, method = :finalize)
    for assocs in Dependency[id]
      assocs.delete_if do |d, m, *o|
	if d == dependant && m == method
	  d.send(m, *o)
	  true
	else
	  false
	end
      end
      Dependency.delete(id) if assoc.empty?
    end
  end
  alias finalize finalize_dependency

  # ��¸��Ϣ R_*(id, dependtant) �Ƿ�Ф��dependant��finalize����.
  def finalize_all_dependency(id, dependant)
    for assoc in Dependency[id]
      assoc.delete_if do |d, m, *o|
	if d == dependant
	  d.send(m, *o)
	  true
	else
	  false
	end
      end
      Dependency.delete(id) if assoc.empty?
    end
  end

  # ��¸��Ϣ R_method(*, dependtant) �Ƿ�Ф��dependant��finalize����.
  def finalize_by_dependant(dependant, method = :finalize)
    for id in Dependency.keys
      finalize(id, dependant, method)
    end
  end

  # ��¸��Ϣ R_*(*, dependtant) �Ƿ�Ф��dependant��finalize����.
  def fainalize_all_by_dependant(dependant)
    for id in Dependency.keys
      finalize_all_dependency(id, dependant)
    end
  end

  # Finalizer����Ͽ����Ƥ������Ƥ�dependant��finalize����
  def finalize_all
    for id, assocs in Dependency
      for dependant, method, *opt in assocs
	dependant.send(method, id, *opt)
      end
      assocs.clear
    end
  end

  # finalize_* ������˸ƤӽФ�����Υ��ƥ졼��
  def safe
    old_status, Thread.critical = Thread.critical, true
    ObjectSpace.remove_finalizer(Proc)
    begin
      yield
    ensure
      ObjectSpace.add_finalizer(Proc)
      Thread.critical = old_status
    end
  end

  # ObjectSpace#add_finalizer�ؤ���Ͽ�ؿ�
  def final_of(id)
    if assocs = Dependency.delete(id)
      for dependant, method, *opt in assocs
	dependant.send(method, id, *opt)
      end
    end
  end

  Proc = proc{|id| final_of(id)}
  ObjectSpace.add_finalizer(Proc)

  module_function :add
  module_function :add_dependency

  module_function :delete
  module_function :delete_dependency
  module_function :delete_all_dependency
  module_function :delete_by_dependant
  module_function :delete_all_by_dependant

  module_function :finalize
  module_function :finalize_dependency
  module_function :finalize_all_dependency
  module_function :finalize_by_dependant
  module_function :fainalize_all_by_dependant
  module_function :finalize_all

  module_function :safe

  module_function :final_of
  private_class_method :final_of

end
