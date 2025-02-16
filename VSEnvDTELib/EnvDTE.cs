using EnvDTE;

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
	void build_if_needed();
	void up_to_date(IntPtr p_val);
}

// COM-visible class implementing the interface.
[ComVisible(true)]
[Guid("6BA04EBF-3841-4A5E-AF68-8DFD4E038A00")]
[ClassInterface(ClassInterfaceType.None)]
[ProgId("VSEnvDTELib.VSEnvDTE")]
public class VSEnvDTE : IVSEnvDTE
{
	private EnvDTE80.DTE2 _dte2 = null;
	private ProjectItem _pitem_components_h = null;
	private dynamic _vc_config = null;
	private static readonly string _prog_id = "VisualStudio.DTE";//   .16.0";
	private IntPtr _p_vs_opened = IntPtr.Zero;

	private string _log_path = Environment.GetFolderPath(Environment.SpecialFolder.Desktop) + "c_sharp_log.txt";

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

			File.WriteAllText(_log_path, "========================\n");
			if (_dte2.Solution.IsOpen is false)
			{
				File.AppendAllText(_log_path, String.Format("sln path : ", sln_path, "\n"));
				_dte2.Solution.Open(sln_path);
			}

			_dte2.UserControl = true;
			_dte2.MainWindow.Activate();
			_dte2.MainWindow.SetFocus();

			var hwnd = _dte2.MainWindow.HWnd;

			_dte2.Events.SolutionEvents.AfterClosing += on_vs_closing;

			foreach (ProjectItem p_item in _dte2.Solution.Projects.Item(1).ProjectItems)
			{
				if (p_item.Name == "components.h")
				{
					_pitem_components_h = p_item;
					break;
				}
			}

			if (_pitem_components_h is null)
			{
				throw new Exception("cannot find components.h");
			}

			_dte2.Events.DocumentEvents[null].DocumentSaved += (d) => { };

			var proj = _dte2.Solution.Projects.Item(1);
			var vcproj = proj.Object;// as VCProject;
			var configs = vcproj.Configurations;// as IVCCollection;
			_vc_config = configs.Item(1);// as VCConfiguration;

			if (_vc_config is null)
			{
				throw new Exception("cannot find _vc_config");
			}

			File.AppendAllText(_log_path, "successed");
		}
		catch (Exception ex)
		{
			File.AppendAllText(_log_path, ex.ToString());
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

	public void build_if_needed()
	{
		if (_dte2 is null || _vc_config is null)
		{
			return;
		}

		if (_vc_config.UpToDate is false)
		{
			//_dte2.ExecuteCommand("Build.BuildSolution");

			var sb = _dte2.Solution.SolutionBuild;
			sb.SolutionConfigurations.Item("DebugEditor").Activate();
			sb.Build(true);
		}
	}

	public void up_to_date(IntPtr p_val)
	{
		if (_vc_config is not null)
		{
			Marshal.WriteByte(p_val, _vc_config.UpToDate ? (byte)1 : (byte)0);
		}
	}

	private void on_vs_closing()
	{
		if (_p_vs_opened != IntPtr.Zero)
		{
			Marshal.WriteByte(_p_vs_opened, 0);
		}

		deinit();
	}

	private void deinit()
	{

		_dte2.Events.SolutionEvents.AfterClosing -= on_vs_closing;
		_dte2 = null;
		_pitem_components_h = null;
	}
}