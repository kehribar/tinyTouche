	ihsan Kehribar
	June 2012

	Basic SVM classifier + OpenGL visualiser + firmware for tinyTouche project.
	https://github.com/kehribar/tinyTouche

	This project is based on Disney Touché 
	http://www.disneyresearch.com/research/projects/hci_touche_drp.htm

	Usage	of the libSVM and the most of the practical information and
	some code snippets are taken from the Sprite_tm's engarde project.
	http://spritesmods.com/?art=engarde
		
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
			 
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
					 
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

* Training	
	- Run the training program, follow the orders ,how many classes needed,
	how many samples required for each sample, and this program generates
	the training file for you.
	- Generate the model using this training file by using 
	'svm-easy train.svm' command

