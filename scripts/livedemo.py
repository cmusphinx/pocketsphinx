#!/usr/bin/env python

# Copyright (c) 2007 Carnegie Mellon University.
#
# You may modify and redistribute this file under the same terms as
# the CMU Sphinx system.  See
# http://cmusphinx.sourceforge.net/html/LICENSE for more information.
#
# Briefly, don't remove the copyright.  Otherwise, do what you like.

__author__ = "David Huggins-Daines <dhuggins@cs.cmu.edu>"

# System imports
import sys

import pygtk
pygtk.require('2.0')
import gobject
import pango
import gtk

import pygst
pygst.require('0.10')
gobject.threads_init()
import gst

class SpeechResult(gtk.DrawingArea):
    def __init__(self, fontsize=24):
        gtk.DrawingArea.__init__(self)
        self.connect("expose_event", self.expose)
        self.layout = pango.Layout(self.get_pango_context())
        self.desc = pango.FontDescription()
        self.fontsize = fontsize
        self.desc.set_family('Sans')
        self.desc.set_size(int(self.fontsize * 1024))
        self.layout.set_font_description(self.desc)
        self.layout.set_text("This is a test")
        ink, log = self.layout.get_pixel_extents()
        self.set_size_request(log[2], log[3])
        self.layout.set_text("")

    def set_hyp(self, hyp):
        # Invalidate the previous hypothesis' extents
        rect = self.get_allocation()
        ink, log = self.layout.get_pixel_extents()
        self.queue_draw_area(ink[0] + rect.x, ink[1] + rect.y,
                             ink[2], ink[3])
        self.layout.set_text(hyp)
        # Try to expand this widget's size if needed
        if log[2] > rect.width or log[3] > rect.height:
            width = max(log[2], rect.width)
            height = max(log[3], rect.height)
            self.set_size_request(width, height)
        # Invalidate the new hypothesis' extents
        ink, log = self.layout.get_pixel_extents()
        self.queue_draw_area(ink[0] + rect.x, ink[1] + rect.y,
                             ink[2], ink[3])

    def expose(self, widget, event):
        # Get a graphics context and clip to the event region
        context = widget.window.cairo_create()
        context.rectangle(event.area.x, event.area.y,
                          event.area.width, event.area.height)
        context.clip()
        # Fill a white background
        rect = self.get_allocation()
        context.rectangle(rect)
        context.set_source_rgb(1,1,1)
        context.fill()
        # Display updated hypothesis in black
        context.set_source_rgb(0,0,0)
        context.move_to(rect.x, rect.y)
        context.show_layout(self.layout)
        return True

class KineticDragPort(gtk.Viewport):
    """
    Class implementing a viewport with horizontal kinetic scrolling.

    FIXME: For some reason this isn't double-buffered properly...
    """
    NO_DRAG = 0  #: No dragging is going on
    PRE_DRAG = 1 #: Button down, dragging might happen
    IN_DRAG = 2  #: Button down, dragging in progress

    def __init__(self, decay_factor=0.95, timeout=10):
        gtk.Viewport.__init__(self)
        self.decay_factor = decay_factor
        self.timeout = timeout
        self.add_events(gtk.gdk.BUTTON_PRESS_MASK
                        | gtk.gdk.BUTTON_RELEASE_MASK
                        | gtk.gdk.POINTER_MOTION_MASK)
        self.connect_after("button_press_event", self.button_press)
        self.connect_after("button_release_event", self.button_release)
        self.connect_after("size_allocate", self.handle_size_allocate)
        self.connect_after("add", self.handle_add)
        self.drag_state = self.NO_DRAG
        self.motion_notify_id = 0
        self.timeout_id = 0

    def handle_add(self, widget, child):
        # Record child size, so we can set our adjustments accordingly.
        child_size = child.size_request()
        hadj = self.get_hadjustment()
        hadj.lower = 0
        hadj.upper = child_size[1]
        return False

    def handle_size_allocate(self, widget, allocation):
        hadj = self.get_hadjustment()
        hadj.page_size = allocation.width
        return False

    def button_press(self, widget, event):
        self.drag_state = self.PRE_DRAG
        self.drag_x = event.x
        if self.motion_notify_id == 0:
            self.motion_notify_id = self.connect_after("motion_notify_event",
                                                       self.motion_notify)
        if self.timeout_id:
            gobject.source_remove(self.timeout_id)
            self.timeout_id = 0
        return True

    def button_release(self, widget, event):
        if self.drag_state == self.IN_DRAG:
            # Calculate average delta over this drag
            self.delta_x /= self.n_deltas
            # Set up a timeout function to do kinetic scrolling, with decaying deltas
            if self.timeout_id:
                gobject.source_remove(self.timeout_id)
            self.timeout_id = \
                            gobject.timeout_add(self.timeout,
                                                self.drag_decay)
        self.drag_state = self.NO_DRAG
        if self.motion_notify_id:
            self.disconnect(self.motion_notify_id)
            self.motion_notify_id = 0
        return True

    def drag_decay(self):
        self.delta_x *= self.decay_factor
        # Decayed too far
        if abs(self.delta_x) < 1:
            self.timeout_id = 0
            return False
        # Ran up against edge of child
        if self.scroll_hadj(self.delta_x):
            self.timeout_id = 0
            return False
        return True

    def motion_notify(self, widget, event):
        if self.drag_state == self.PRE_DRAG:
            self.drag_state = self.IN_DRAG
            self.delta_x = 0.
            self.n_deltas = 0
        if self.drag_state == self.IN_DRAG:
            delta_x = event.x - self.drag_x
            self.drag_x = event.x
            self.delta_x += delta_x
            self.n_deltas += 1
            self.scroll_hadj(delta_x)
        return True

    def scroll_hadj(self, delta_x):
        clipped = False
        hadj = self.get_hadjustment()
        value = hadj.value - delta_x
        if value < 0:
            value = 0
            clipped = True
        if value > hadj.upper - hadj.page_size:
            value = hadj.upper - hadj.page_size
            clipped = True
        hadj.value = value
        return clipped

class LiveDemo(object):
    def __init__(self):
        # Create toplevel window
        self.window = gtk.Window()
        self.window.connect("delete-event", gtk.main_quit)

        # Add a result area and dragport
        self.result = SpeechResult()
        scroll = KineticDragPort()
        scroll.set_size_request(600, 200)
        scroll.add(self.result)

        # And a push to talk button
        vbox = gtk.VBox()
        vbox.pack_start(scroll)
        self.button = gtk.ToggleButton("speak")
        self.button.connect('clicked', self.button_clicked)
        vbox.pack_start(self.button)
        self.window.add(vbox)

        # Create ASR pipeline
        self.pipeline = gst.parse_launch('gconfaudiosrc ! audioresample ! vader name=vad '
                                         + '! audioconvert ! pocketsphinx name=asr ! fakesink')
        asr = self.pipeline.get_by_name('asr')
        asr.set_property('maxhmmpf', 500)
        asr.set_property('maxwpf', 5)
        asr.set_property('dsratio', 2)

        # Connect some signal handlers to ASR signals (will use the bus to forward them)
        asr.connect('partial_result', self.asr_partial_result)
        asr.connect('result', self.asr_result)

        # Connect VADER signals
        vader = self.pipeline.get_by_name('vad');
        vader.set_property('threshold', 0.1)
        vader.connect('vader_start', self.vader_start)
        vader.connect('vader_stop', self.vader_stop)

        # Bus signals etc
        bus = self.pipeline.get_bus()
        bus.add_signal_watch()
        bus.connect('message::application', self.application_message)

        self.pipeline.set_state(gst.STATE_PAUSED)
        self.window.show_all()

    def button_clicked(self, button):
        if button.get_active():
            button.set_label("Pause")
            self.pipeline.set_state(gst.STATE_PLAYING)
        else:
            # FIXME: Signal an end of utterance somehow
            self.pipeline.set_state(gst.STATE_PAUSED)

    def application_message(self, bus, msg):
        msgtype = msg.structure.get_name()
        if msgtype == 'partial_result':
            self.result.set_hyp(msg.structure['hyp'])
        elif msgtype == 'result':
            self.result.set_hyp(msg.structure['hyp'])
        elif msgtype == 'vader_stop':
            self.pipeline.set_state(gst.STATE_PAUSED)
            self.button.set_label("Speak")
            self.button.set_active(False)

    def vader_start(self, vader, ts):
        """
        Forward VADER start signals on the bus to the main thread.
        """
        struct = gst.Structure('vader_start')
        # Ignore the timestamp for now because gst-python is buggy
        vader.post_message(gst.message_new_application(vader, struct))

    def vader_stop(self, vader, ts):
        """
        Forward VADER stop signals on the bus to the main thread.
        """
        struct = gst.Structure('vader_stop')
        # Ignore the timestamp for now because gst-python is buggy
        vader.post_message(gst.message_new_application(vader, struct))

    def asr_partial_result(self, asr, text):
        """
        Forward partial result signals on the bus to the main thread.
        """
        struct = gst.Structure('partial_result')
        struct.set_value('hyp', text)
        asr.post_message(gst.message_new_application(asr, struct))

    def asr_result(self, asr, text):
        """
        Forward result signals on the bus to the main thread.
        """
        struct = gst.Structure('result')
        struct.set_value('hyp', text)
        asr.post_message(gst.message_new_application(asr, struct))

d = LiveDemo()
gtk.main()
