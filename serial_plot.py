#!/usr/bin/env python2
# -*- coding: utf-8 -*-
#
# Real time plot of serial data
#
# Copyright (C) 2012 Asaf Paris Mandoki http://asaf.pm
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import pygame
from pygame.locals import *
from numpy import array, arange, zeros, roll

import threading
from serial import Serial
from struct import unpack

import sys

pygame.init()

global lock
lock = threading.Lock()

pushup_count = squat_count = 0
bad=False


""" serial data structure:

   For synchronization purposes, the following scheme was chosen:
   A0 data:   A09 (MSB) A08 A07 A06 A05 A04 A03 A02 A01 A00 (LSB)
   sent as byte 1:   1 1 1 A09 A08 A07 A06 A05
       and byte 2:   0 1 1 A04 A03 A02 A01 A00

           byte 1  A0 5 most significant bits + 224 (128+64+32), legitimate values are between 224 and 255
           byte 2  A0 5 least significant bits + 96 (64+32)    , legitimate values are between 96 and 127
"""

global freeform
freeform = False

class getInput(threading.Thread):
    stopthread = threading.Event()
    def __init__(self):
        threading.Thread.__init__(self)
        self.start()
    
    def run(self):
        global freeform
        while not self.stopthread.isSet() :
            raw_input("switch freeform?\n")
            freeform = not freeform

            
class DataReader(threading.Thread):
        
    #Thread event, stops the thread if it is set.
    stopthread = threading.Event()
    
    def __init__(self):
        threading.Thread.__init__(self)                     #Call constructor of parent
        self.ser = Serial(
                port = "/dev/serial/by-id/usb-FTDI_FT232R_USB_UART_AJ02ZJVW-if00-port0",
                baudrate=9600)            #Initialize serial port
        self.data_buff_size = 200                           #Buffer size
        self.data1 = zeros(self.data_buff_size)              #Data buffer
        self.data2 = zeros(self.data_buff_size)              #Data buffer
        self.data3 = zeros(self.data_buff_size)              #Data buffer
        self.data4 = zeros(self.data_buff_size)              #Data buffer
        self.all_data = ""
        self.start()
    
    def run(self):      #Run method, this is the code that runs while thread is alive.
        global bad, squat_count, pushup_count
        num_bytes = 400                                     #Number of bytes to read at once
        val = 0                                             #Read value
        itor = 0
        
        while not self.stopthread.isSet() :
            rslt = self.ser.readline()
            try:
                line = rslt.split(',')
                y1,y2,y3,y4,squat,pushup,bad = [float(l) for l in line]
            except:
                print rslt
                continue
            if squat > squat_count:
                squat_count = squat
            if pushup > pushup_count:
                pushup_count = pushup


            itor += 1
            lock.acquire()
            self.all_data += rslt
            self.data1 = roll(self.data1,-1)
            self.data1[-1] = y1
            
            self.data2 = roll(self.data2,-1)
            self.data2[-1] = y2

            self.data3 = roll(self.data3,-1)
            #self.data3[-1] =(2.0/3.0)* y3
            self.data3[-1] = (0.5)*y3

            self.data4 = roll(self.data4,-1)
            self.data4[-1] = y4

            lock.release()
            #print y1,y2,y3

        """
            if itor % 1000 == 0:
                itor = 0
                with open("results.csv", "wb") as f:
                    f.write(self.all_data)


         
        self.ser.close()
        with open("results.csv", "wb") as f:
            f.write(self.all_data)
        """
        self.ser.close()
            
    def stop(self):
        self.stopthread.set()

class Oscilloscope():
    
    def __init__(self):
        self.w,self.h = (640*2, 600)
        self.screen = pygame.display.set_mode((self.w,self.h))
        self.clock = pygame.time.Clock()
        self.data_reader = DataReader()
        self.run()


        
    def plot(self, x, y, xmin, xmax, ymin, ymax,color):
        w, h =(self.w,480) # self.screen.get_size()
        x = array(x)
        y = array(y)
        
        #Scale data
        xspan = abs(xmax-xmin)
        yspan = abs(ymax-ymin)
        xsc = 1.0*(w+1)/xspan
        ysc = 5*h/yspan
        xp = (x-xmin)*xsc
        yp = h-(y-ymin)*ysc
        
        #Draw grid
        for i in range(10):
            pygame.draw.line(self.screen, (210, 210, 210), (0,int(h*0.1*i)), (w-1,int(h*0.1*i)), 1)
            pygame.draw.line(self.screen, (210, 210, 210), (int(w*0.1*i),0), (int(w*0.1*i),h-1), 1)
            
        #Plot data
        for i in range(len(xp)-1):
            pygame.draw.line(self.screen, color, (int(xp[i]), int(yp[i])), 
                                                     (int(xp[i+1]),int(yp[i+1])), 5)
            


    def run(self):
        global pushup_count, squat_count,bad, freeform


        #Things we need in the main loop
        font = pygame.font.Font(pygame.font.match_font(u'mono'), 20)
        data_buff_size = self.data_reader.data_buff_size        
        hold = False
        self.freeform = False

        while 1:
            #Process events
            event = pygame.event.poll()
            if event.type == pygame.QUIT:
                pygame.quit()
                self.data_reader.stop()
                sys.exit()
            """if event.type == pygame.KEYDOWN :
                if event.key == pygame.K_RETURN:
                    print "entering freeform!"
                    self.freeform = not self.freeform"""
            if event.type == pygame.KEYDOWN :
                if event.key == pygame.K_h:
                    hold = not hold
                    
                    
            self.screen.fill((255,255,255))     

            # Plot current buffer
            if not hold:
                lock.acquire()
                x = arange(data_buff_size)
                y1 = self.data_reader.data1
                y2 = self.data_reader.data2
                y3 = self.data_reader.data3
                y4 = self.data_reader.data4
                lock.release()

            self.plot(x,y1, 0, data_buff_size, 0, 1024, (0,0,255))
            self.plot(x,y2, 0, data_buff_size, 0, 1024, (255,0,0))
            self.plot(x,y3, 0, data_buff_size, 0, 1024,  (0,255,0))
            self.plot(x,y4, 0, data_buff_size, 0, 1024,  (255,0,255))

            # Display fps
            font = pygame.font.Font(pygame.font.match_font(u'mono'), 60)
            font.set_bold(True)
            text1= font.render("ActiveTrac", True, (255, 10, 10))
            self.screen.blit(text1, (10, 10))
            
            newfont = pygame.font.Font(None, 70)
            text2 = newfont.render("Toe", True, (255, 0, 0))
            text3 = newfont.render("Hand", True, (0, 255, 0))
            text4 = newfont.render("Heel", True, (0, 0, 255))
            text5 = newfont.render("Accel", True, (255,0,255))
            self.screen.blit(text2, (self.w*.85, 60))
            self.screen.blit(text3, (self.w*.85, 110))
            self.screen.blit(text4, (self.w*.85, 10))
            self.screen.blit(text5, (self.w*.85, 160))
            
            if bad and not freeform:
                back = (255,0,0)
            else:
                back=(0,0,0)
            pygame.draw.rect(self.screen, back, (0, 480, self.w, 600-480))
            if  freeform:
                font = pygame.font.Font(None, 80)
                text7= font.render("Freeform", True, (255,255,255))
                self.screen.blit(text7, (self.w*.45, 500))
            

            else:
                font = pygame.font.Font(None, 80)
                text7= font.render("Pushups: %d   Squats: %d" %(pushup_count,squat_count), True, (255,255,255))
                self.screen.blit(text7, (self.w*.3, 500))
            

            pygame.display.flip()
            self.clock.tick(0)
getInput()
osc = Oscilloscope()
print "hello"
q = raw_input()
print q
