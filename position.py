#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
#  position.py
#  
#  Copyright 2018 Stefan Olin <stefan@karibu>
#  
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#  
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#  
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#  
#  

'''
A class holding latitude and longitude for a position.
The name of the position will be created from lon and lat if not given.
Instances of the class are hashable and can be used as indexes in 
dictionaries. It is also possible to use set.
'''
class position():
	def __init__(self, longitude, latitude, name=None):
		self.lon = longitude
		self.lat = latitude
		self.key = str(self.lon)+"_"+str(self.lat)
		if name!=None:
			self.name = name
		else:
			self.name = self.key
	def __hash__(self):
		return hash(self.key)
	def __eq__(self,other):
		if self.__class__ == other.__class__:
			if float(self.lon) == float(other.lon):
				if float(self.lat) == float(other.lat):
					return True
				else:
					return False
			else:
				return False
		else:
			return False
		
	def __str__(self):
		return str(self.key)
