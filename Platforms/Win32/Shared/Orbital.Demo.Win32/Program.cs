﻿using System;
using Orbital.Host;
using Orbital.Host.Win32;

namespace Orbital.Demo.Win32
{
	static class Program
	{
		static void Main(string[] args)
		{
			var application = new Application();
			var window = new Window(0, 0, 320, 240, WindowSizeType.WorkingArea, WindowType.Tool, WindowStartupPosition.CenterScreen);
			window.SetTitle("Demo: Win32");
			window.Show();
			application.Run(window);
		}
	}
}