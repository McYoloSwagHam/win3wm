using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Interop;
using System.Runtime.InteropServices; /// We need this...
using System.Threading;
using System.Runtime.CompilerServices;
using System.IO.Pipes;
using System.IO;
using System.Diagnostics;

namespace WinWMGUI
{

    class GUI
    {
        public static MainWindow mainview_ui;
        public static NamedPipeClientStream gui_pipe;
        public static IntPtr mainview_handle = IntPtr.Zero;
        public static Thread gui_thread;

        static public void Main(String[] Args)
        {


            gui_thread = new Thread(() =>
            {
                gui_pipe = new NamedPipeClientStream(".", "winwm_gui", PipeDirection.InOut);
                gui_pipe.Connect(5000);

                mainview_ui = new MainWindow(gui_pipe);
                mainview_ui.Show();
                System.Windows.Threading.Dispatcher.Run();
            });

            gui_thread.SetApartmentState(ApartmentState.STA); /// STA Thread Initialization
            gui_thread.Start();

        }

    }

    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {

        NamedPipeClientStream gui_pipe;

        private void ButtonHandler(object Sender, RoutedEventArgs Event)
        {
            StreamWriter writer = new StreamWriter(gui_pipe);
            writer.Write(UserBox.Text);
            writer.Flush();
            this.Close();
        }

        public MainWindow(NamedPipeClientStream connection)
        {
            gui_pipe = connection;
            InitializeComponent();
        }
    }
}
