using EnvDTE;

using System;
using System.IO;
using System.Linq;
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
	void init([MarshalAs(UnmanagedType.BStr)] string sln_path);
	void open_vs();
	void monitor_vs_opened(IntPtr p_val);
	void build_if_needed();
	void up_to_date(IntPtr p_val);
	void edit(
		[MarshalAs(UnmanagedType.BStr)] string begin_path,
		[MarshalAs(UnmanagedType.BStr)] string end_path,
		[MarshalAs(UnmanagedType.BStr)] string replace,
		[MarshalAs(UnmanagedType.BStr)] string search_end,
		[MarshalAs(UnmanagedType.I1)] bool repeat);
	void deinit();
}

// COM-visible class implementing the interface.
[ComVisible(true)]
[Guid("6BA04EBF-3841-4A5E-AF68-8DFD4E038A00")]
[ClassInterface(ClassInterfaceType.None)]
[ProgId("VSEnvDTELib.VSEnvDTE")]
public class VSEnvDTE : IVSEnvDTE
{
	private static readonly string _prog_id = "VisualStudio.DTE";//   .16.0";
	private static readonly string _log_path = Environment.GetFolderPath(Environment.SpecialFolder.Desktop) + "c_sharp_log.txt";

	private EnvDTE80.DTE2 _dte2 = null;
	private string _sln_path = null;
	private ProjectItem _pitem_components_h = null;
	private dynamic _vc_config = null;
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

	public void init([MarshalAs(UnmanagedType.BStr)] string sln_path)
	{
		IRunningObjectTable rot = null;
		IEnumMoniker enum_moniker = null;
		IMoniker dte2_mk = null;

		try
		{
			_sln_path = sln_path;
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

			//if (_dte2.Solution.IsOpen is false)
			//{
			//	_dte2.Solution.Open(sln_path);
			//}

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


			var proj = _dte2.Solution.Projects.Item(1);
			var vcproj = proj.Object;// as VCProject;
			var configs = vcproj.Configurations;// as IVCCollection;
			_vc_config = configs.Item(1);// as VCConfiguration;

			if (_vc_config is null)
			{
				throw new Exception("cannot find _vc_config");
			}
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

	public void open_vs()
	{
		_dte2.UserControl = true;

		if (_dte2.Solution.IsOpen is false)
		{
			_dte2.Solution.Open(_sln_path);
		}
		_dte2.MainWindow.Activate();
		_dte2.MainWindow.SetFocus();
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

	public void edit(
		[MarshalAs(UnmanagedType.BStr)] string begin_path,
		[MarshalAs(UnmanagedType.BStr)] string end_path,
		[MarshalAs(UnmanagedType.BStr)] string replace,
		[MarshalAs(UnmanagedType.BStr)] string search_end,
		[MarshalAs(UnmanagedType.I1)] bool repeat)
	{
		var text_doc = _pitem_components_h.Document.Object("TextDocument") as TextDocument;
		var temp = text_doc.StartPoint.CreateEditPoint().GetText(text_doc.EndPoint);
		var full_text = temp.AsSpan();
		var text_selection = text_doc.Selection;
		var path_splitted = begin_path.Split('@');

		var begin_idx = 0;
		var end_idx = 0;

		foreach (var sub_path in path_splitted.Take(path_splitted.Length - 1))
		{
			var sub = full_text.Slice(begin_idx);
			var idx = sub.IndexOf(sub_path);

			if (idx == -1)
			{
				return;
			}

			begin_idx += idx;
		}
		var search_end_idx = begin_idx + full_text.Slice(begin_idx).IndexOf(search_end);

		if (search_end_idx == begin_idx - 1)
		{
			return;
		}

		full_text = full_text.Slice(0, search_end_idx);

		var abs_offset = 1;
		var replace_length = replace.Length - replace.Count(c => c == '\r');
		do
		{
			var begin_offset = 0;
			var end_offset = 0;

			{
				var sub = full_text.Slice(begin_idx);
				var idx = sub.IndexOf(path_splitted.LastOrDefault());

				if (idx == -1)
				{
					return;
				}

				begin_idx += idx;
			}

			{
				var idx = full_text.Slice(begin_idx).IndexOf(end_path);

				if (idx == -1)
				{
					break;
				}

				end_idx = begin_idx + idx + end_path.Length;
			}

			foreach (var c in full_text.Slice(0, begin_idx))
			{
				if (c == '\r')
				{
					++begin_offset;
				}
			}

			foreach (var c in full_text.Slice(begin_idx, end_idx - begin_idx))
			{
				if (c == '\r')
				{
					++end_offset;
				}
			}

			end_offset += begin_offset;

			text_selection.MoveToAbsoluteOffset(begin_idx - begin_offset + abs_offset, false);
			//text_selection.StartOfLine(vsStartOfLineOptions.vsStartOfLineOptionsFirstColumn);
			text_selection.MoveToAbsoluteOffset(end_idx - end_offset + abs_offset, true);
			//text_selection.EndOfLine(true);
			text_selection.Text = replace;

			var abs_begin = begin_idx - begin_offset + abs_offset;
			var abs_end = end_idx - end_offset + abs_offset;
			var remove_length = abs_end - abs_begin + 1;

			abs_offset += -remove_length + replace_length;
			begin_idx = end_idx;
		} while (repeat);

		//_pitem_components_h.Save();
		//_dte2.ExecuteCommand("File.SaveAll");
	}

	public void deinit()
	{
		_dte2.Events.SolutionEvents.AfterClosing -= on_vs_closing;

		_dte2 = null;
		_sln_path = null;
		_pitem_components_h = null;
		_vc_config = null;
		_p_vs_opened = IntPtr.Zero;
	}

	private void on_vs_closing()
	{
		if (_p_vs_opened != IntPtr.Zero)
		{
			Marshal.WriteByte(_p_vs_opened, 0);
		}
	}
}