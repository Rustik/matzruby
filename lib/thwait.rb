#
#   thwait.rb - ����å�Ʊ�����饹
#   	$Release Version: 0.9 $
#   	$Revision: 1.3 $
#   	$Date: 1998/06/26 03:19:34 $
#   	by Keiju ISHITSUKA(Nihpon Rational Software Co.,Ltd.)
#
# --
#  ��ǽ:
#  ʣ���Υ���åɤ�ؤ������Υ���åɤ���λ����ޤ�wait���뵡ǽ����
#  ������. 
#
#  ���饹�᥽�å�:
#  * ThreadsWait.all_waits(thread1,...)
#    ���ƤΥ���åɤ���λ����ޤ��Ԥ�. ���ƥ졼���Ȥ��ƸƤФ줿���ˤ�, 
#    ����åɤ���λ�����٤˥��ƥ졼����¹Ԥ���.
#  * th = ThreadsWait.new(thread1,...)
#    Ʊ�����륹��åɤ���ꤷƱ�����֥������Ȥ�����.
#  
#  �᥽�å�:
#  * th.threads
#    Ʊ�����٤�����åɤΰ���
#  * th.empty?
#    Ʊ�����٤�����åɤ����뤫�ɤ���
#  * th.finished?
#    ���Ǥ˽�λ��������åɤ����뤫�ɤ���
#  * th.join(thread1,...) 
#    Ʊ�����륹��åɤ���ꤷ, �����줫�Υ���åɤ���λ����ޤ��Ԥ��ˤϤ���.
#  * th.join_nowait(threa1,...)
#    Ʊ�����륹��åɤ���ꤹ��. �Ԥ��ˤ�����ʤ�.
#  * th.next_wait
#    �����줫�Υ���åɤ���λ����ޤ��Ԥ��ˤϤ���.
#  * th.all_waits
#    ���ƤΥ���åɤ���λ����ޤ��Ԥ�. ���ƥ졼���Ȥ��ƸƤФ줿���ˤ�, 
#    ����åɤ���λ�����٤˥��ƥ졼����¹Ԥ���.
#

require "thread.rb"
require "e2mmap.rb"

class ThreadsWait
  RCS_ID='-$Id: thwait.rb,v 1.3 1998/06/26 03:19:34 keiju Exp keiju $-'
  
  Exception2MessageMapper.extend_to(binding)
  def_exception("ErrNoWaitingThread", "No threads for waiting.")
  def_exception("ErrNoFinshedThread", "No finished threads.")
  
  # class mthods
  #	all_waits
  
  #
  # ���ꤷ������åɤ����ƽ�λ����ޤ��Ԥ�. ���ƥ졼���Ȥ��ƸƤФ���
  # ���ꤷ������åɤ���λ����Ȥ��ν�λ��������åɤ�����Ȥ��ƥ��ƥ졼
  # ����ƤӽФ�. 
  #
  def ThreadsWait.all_waits(*threads)
    tw = ThreadsWait.new(*threads)
    if iterator?
      tw.all_waits do
	|th|
	yield th
      end
    else
      tw.all_waits
    end
  end
  
  # initialize and terminating:
  #	initialize
  
  #
  # �����. �Ԥĥ���åɤλ��꤬�Ǥ���.
  #
  def initialize(*threads)
    @threads = []
    @wait_queue = Queue.new
    join_nowait(*threads) unless threads.empty?
  end
  
  # accessing
  #	threads
  
  # �Ԥ�����åɤΰ������֤�.
  attr :threads
  
  # testing
  #	empty?
  #	finished?
  #
  
  #
  # �Ԥ�����åɤ�¸�ߤ��뤫�ɤ������֤�.
  def empty?
    @threads.empty?
  end
  
  #
  # ���Ǥ˽�λ��������åɤ����뤫�ɤ����֤�
  def finished?
    !@wait_queue.empty?
  end
  
  # main process:
  #	join
  #	join_nowait
  #	next_wait
  #	all_wait
  
  #
  # �ԤäƤ��륹��åɤ��ɲä�. �����줫�Υ���åɤ�1�Ľ�λ����ޤ���
  # ���ˤϤ���.
  #
  def join(*threads)
    join_nowait(*threads)
    next_wait
  end
  
  #
  # �ԤäƤ��륹��åɤ��ɲä���. �Ԥ��ˤ�����ʤ�.
  #
  def join_nowait(*threads)
    @threads.concat threads
    for th in threads
      Thread.start do
	th = Thread.join(th)
	@wait_queue.push th
      end
    end
  end
  
  #
  # �����줫�Υ���åɤ���λ����ޤ��Ԥ��ˤϤ���.
  # �ԤĤ٤�����åɤ��ʤ����, �㳰ErrNoWaitingThread���֤�.
  # nonnlock�����λ��ˤ�, nonblocking��Ĵ�٤�. ¸�ߤ��ʤ����, �㳰
  # ErrNoFinishedThread���֤�.
  #
  def next_wait(nonblock = nil)
    ThreadsWait.fail ErrNoWaitingThread if @threads.empty?
    begin
      @threads.delete(th = @wait_queue.pop(nonblock))
      th
    rescue ThreadError
      ThreadsWait.fail ErrNoFinshedThread
    end
  end
  
  #
  # ���ƤΥ���åɤ���λ����ޤ��Ԥ�. ���ƥ졼���Ȥ��ƸƤФ줿����, ��
  # ��åɤ���λ�����٤�, ���ƥ졼����ƤӽФ�.
  #
  def all_waits
    until @threads.empty?
      th = next_wait
      yield th if iterator?
    end
  end
end

ThWait = ThreadsWait
