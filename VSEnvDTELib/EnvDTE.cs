using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;

namespace VSEnvDTELib;

// COM-visible interface with a unique GUID.
[ComVisible(true)]
[Guid("CE374261-7F7E-4FB8-AF6B-7C33B8F06D9C")]
[InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
public interface IVSEnvDTE
{
	//int Init();
	void open_vs([MarshalAs(UnmanagedType.BStr)] string sln_path);
	void monitor_vs_opened(IntPtr p_val);
}

// COM-visible class implementing the interface.
[ComVisible(true)]
[Guid("6BA04EBF-3841-4A5E-AF68-8DFD4E038A00")]
[ClassInterface(ClassInterfaceType.None)]
[ProgId("VSEnvDTELib.VSEnvDTE")]
public class VSEnvDTE : IVSEnvDTE
{
	private EnvDTE80.DTE2 _dte2 = null;
	private static readonly string _prog_id = "VisualStudio.DTE";//   .16.0";
	private IntPtr _p_vs_opened = IntPtr.Zero;

	//public int Init()
	//{

	//}
	[DllImport("ole32.dll")]
	private static extern int GetRunningObjectTable(uint reserved, out IRunningObjectTable pprot);
	[DllImport("ole32.dll")]
	private static extern int CreateClassMoniker(Guid rclsid, out IMoniker ppmk);
	[DllImport("ole32.dll")]
	private static extern int CoDisconnectObject(object punk, uint reserved);

	private static void Throw_on_fail(int hresult, string msg)
	{

		if (hresult < 0) throw new COMException(String.Format(msg, hresult), hresult);
		//new COMException(String.Format(err_msg, hresult));
	}

	public void open_vs([MarshalAs(UnmanagedType.BStr)] string sln_path)
	{
		IRunningObjectTable rot = null;
		IEnumMoniker enum_moniker = null;
		IMoniker dte2_mk = null;

		try
		{
			var dte2_t = Type.GetTypeFromProgID(_prog_id, true);
			if (dte2_t is null)
			{
				throw new COMException("GetTypeFromProgID failed {}");
			}
			Throw_on_fail(GetRunningObjectTable(0, out rot), "GetRunningObjectTable failed {}");
			Throw_on_fail(CreateClassMoniker(dte2_t.GUID, out dte2_mk), "CreateClassMoniker failed {}");
			rot.EnumRunning(out enum_moniker);
			enum_moniker.Reset();

			IMoniker[] curr_mk = new IMoniker[1];
			object punk;
			bool found = false;
			while ((enum_moniker.Next(1, curr_mk, IntPtr.Zero) == 0) && (found is false))
			{
				if (curr_mk[0].IsEqual(dte2_mk) < 0)
				{
					continue;
				}

				if (rot.GetObject(curr_mk[0], out punk) < 0)
				{
					continue;
				}

				_dte2 = punk as EnvDTE80.DTE2;

				if (_dte2 is null)
				{
					continue;
				}

				string fullname = _dte2.Solution.FullName;
				if (fullname == sln_path)
				{
					found = true;
					break;
				}
			}

			if (found is false)
			{
				_dte2 = Activator.CreateInstance(dte2_t) as EnvDTE80.DTE2;
			}

			if (_dte2 is null)
			{
				throw new COMException();
			}
			File.WriteAllText("C:\\Users\\Jonghun\\Desktop\\c_sharp_log.txt", "========================\n");
			if (_dte2.Solution.IsOpen is false)
			{
				File.AppendAllText("C:\\Users\\Jonghun\\Desktop\\c_sharp_log.txt", String.Format("sln path : ", sln_path, "\n"));
				_dte2.Solution.Open(sln_path);
			}

			_dte2.UserControl = true;
			_dte2.MainWindow.Activate();
			_dte2.MainWindow.SetFocus();

			var evnets = _dte2.Events;

			_dte2.Events.SolutionEvents.AfterClosing += on_vs_closing;


			//var windowEvents = evnets.WindowEvents;
			//windowEvents.WindowActivated += OnWindowActivated;
			//windowEvents.WindowClosing += on_vs_closing;

			//Throw_on_fail(CoDisconnectObject(_dte2, 0), "CoDisconnectObject failed"); // this throws exception. i don't know why

			File.AppendAllText("C:\\Users\\Jonghun\\Desktop\\c_sharp_log.txt", "successed");
		}
		catch (Exception ex)
		{
			string log_path = Environment.GetFolderPath(Environment.SpecialFolder.Desktop);
			File.AppendAllText("C:\\Users\\Jonghun\\Desktop\\c_sharp_log.txt", ex.ToString());

		}
		finally
		{
			if (rot is not null) Marshal.ReleaseComObject(rot);
			if (enum_moniker is not null) Marshal.ReleaseComObject(enum_moniker);
			if (dte2_mk is not null) Marshal.ReleaseComObject(dte2_mk);
		}
	}

	public void monitor_vs_opened(IntPtr p_val)
	{
		_p_vs_opened = p_val;
	}

	private void on_vs_closing(/*Window window*/)
	{
		if (_p_vs_opened != IntPtr.Zero)
		{
			Marshal.WriteByte(_p_vs_opened, 0);
		}
	}

	private void OnWindowActivated(EnvDTE.Window gotFocus, EnvDTE.Window lostFocus)
	{

	}
}