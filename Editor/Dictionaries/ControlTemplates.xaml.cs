using System.Windows;

namespace Editor.Dictionaries
{
	public partial class ControlTemplates : ResourceDictionary
	{
		private void On_CloseBtn_Click(object sender, RoutedEventArgs e)
		{
			var window = (Window)((FrameworkElement)sender).TemplatedParent;
			window.Close();
		}

		private void On_RestoreBtn_Click(object sender, RoutedEventArgs e)
		{
			var window = (Window)((FrameworkElement)sender).TemplatedParent;
			window.WindowState = window.WindowState == WindowState.Normal ? WindowState.Maximized : WindowState.Normal;
		}

		private void On_MinimizeBtn_Click(object sender, RoutedEventArgs e)
		{
			var window = (Window)((FrameworkElement)sender).TemplatedParent;
			window.WindowState = WindowState.Minimized;
		}
	}
}
